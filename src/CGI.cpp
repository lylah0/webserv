#include "CGI.hpp"
#include <stdint.h>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>     // open, fcntl
#include <unistd.h>    // write, close, execve
#include <cstdio>      // sprintf
#include <sys/types.h> // pid_t
#include <sys/stat.h>  // open modes
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "ServerConfig.hpp"
#include "LocationConfig.hpp"
#include "SocketUtils.hpp"

static std::string toString(size_t n) {
    std::stringstream ss;
    ss << n;
    return ss.str();
}

std::string getExtension(const std::string &path) {
    size_t pos = path.rfind('.');
    if (pos == std::string::npos)
        return "";
    std::string ext = path.substr(pos);

    while (!ext.empty()) {
        char c = ext[ext.size() - 1];
        if (c == '\r' || c == '\n' || c == ' ' || c == ';')
            ext.erase(ext.size() - 1);
        else
            break;
    }
    return ext;
}

const LocationConfig* findExtensionLocation(const ServerConfig& server,
											const std::string& uri,
											const std::string& resolvedPath,
											const std::string& ext)
{
	std::string uriNoQuery = uri;
	size_t qpos = uriNoQuery.find('?');
	if (qpos != std::string::npos)
		uriNoQuery.erase(qpos);
	if (uriNoQuery.empty() || uriNoQuery[0] != '/')
		uriNoQuery = "/" + uriNoQuery;
	std::string filename;
	size_t lastSlash = resolvedPath.find_last_of('/');
	if (lastSlash == std::string::npos)
		filename = resolvedPath;
	else
		filename = resolvedPath.substr(lastSlash + 1);
	const LocationConfig* best = NULL;
	size_t bestLen = 0;
	for (size_t i = 0; i < server.locations.size(); ++i) {
		const LocationConfig& L = server.locations[i];
		std::string locPath = L.path;
		if (locPath.empty()) locPath = "/";
		if (locPath.size() > 1 && locPath[locPath.size() - 1] == '/')
			locPath.erase(locPath.size() - 1);
		bool prefixMatch = false;
		if (locPath == "/") {
			prefixMatch = true;
		} else if (uriNoQuery.compare(0, locPath.size(), locPath) == 0) {
			if (uriNoQuery.size() == locPath.size() || uriNoQuery[locPath.size()] == '/')
				prefixMatch = true;
		}
		if (!prefixMatch)
			continue;
		if (!ext.empty() && L.cgi.find(ext) != L.cgi.end()) {
			if (locPath.size() > bestLen) {
				bestLen = locPath.size();
				best = &L;
			}
			continue;
		}
		if (!filename.empty()) {
			std::string fileLoc = "/" + filename;
			if (locPath == fileLoc && locPath.size() > bestLen) {
				bestLen = locPath.size();
				best = &L;
			}
		}
	}
	return best;
}

bool isCGIvalid(const LocationConfig& extLoc,
                const std::string& ext,
                const std::string& method)
{
    if (extLoc.path.empty()) {
        std::cerr << "[CGI] extLoc is NULL, not CGI" << std::endl;
        return false;
    }
    if (ext.empty()) {
        std::cerr << "[CGI] Empty extension, not CGI" << std::endl;
        return false;
    }
    bool methodAllowed = false;
    for (size_t i = 0; i < extLoc.methods.size(); i++) {
        if (extLoc.methods[i] == method) {
            methodAllowed = true;
            break;
        }
    }
    if (!methodAllowed) {
        std::cerr << "[CGI] Method not allowed for this location" << std::endl;
        return false;
    }
    std::map<std::string, std::string>::const_iterator it = extLoc.cgi.find(ext);
    if (it == extLoc.cgi.end() || it->second.empty()) {
        std::cerr << "[CGI] No CGI interpreter for extension " << ext << std::endl;
        return false;
    }
    return true;
}

std::string CGI_validation_check(const std::string& fullPath)
{
    struct stat st;
    std::cerr << "FULLPATH VARIABLE : " << fullPath << std::endl;
    if (stat(fullPath.c_str(), &st) < 0)
        return "File non existent";
    if (!S_ISREG(st.st_mode))
        return "Not a regular file";
    if (access(fullPath.c_str(), R_OK) != 0)
        return "Not readable";
    return "CGI validated";
}

CGIEnv buildCGIEnv(const HttpRequest &req,
                   const ServerConfig &server,
                   const std::string &fullPath)
{
    CGIEnv env;

    size_t qpos = req.uri.find('?');
    std::string uriPath  = (qpos != std::string::npos ? req.uri.substr(0, qpos) : req.uri);
    std::string queryStr = (qpos != std::string::npos ? req.uri.substr(qpos + 1) : "");

    std::map<std::string, std::string>::const_iterator ct = req.headers.find("Content-Type");
    std::string contentType = (ct != req.headers.end()) ? ct->second : "";

    if (req.method == "POST" &&
        queryStr.empty() &&
        contentType.find("application/x-www-form-urlencoded") != std::string::npos)
    {
        queryStr = req.body;
    }
    std::string pathInfo = req.uri;
    if (req.uri.size() > uriPath.size())
        pathInfo = req.uri.substr(uriPath.size());
    char cwd[PATH_MAX];
    getcwd(cwd, sizeof(cwd));
    std::string host = req.headers.count("Host") ? req.headers.at("Host") : "";
    size_t colon = host.find(':');
    if (colon != std::string::npos)
        host = host.substr(0, colon);
    env.scriptFilename = std::string(cwd) + "/" + fullPath;
    env.requestMethod    = req.method;
    env.requestUri       = req.uri;
    env.queryString      = queryStr;
    env.contentLength    = (req.method == "POST" ? toString(req.body.size()) : "0");
    env.contentType      = contentType;
    env.serverProtocol   = req.version;
    env.scriptName       = uriPath;
    env.serverName       = host;
    env.serverPort       = toString(server.listen);
    env.gatewayInterface = "CGI/1.1";
    env.pathInfo         = pathInfo;

    for (std::map<std::string, std::string>::const_iterator it = req.headers.begin();
        it != req.headers.end(); ++it)
    {
        std::string key = it->first;
        if (key == "Content-Length")
            continue;
        if (key == "Transfer-Encoding")
            continue;
        for (size_t i = 0; i < key.size(); i++)
            key[i] = (key[i] == '-' ? '_' : std::toupper(key[i]));

        env.httpHeaders["HTTP_" + key] = it->second;
    }
    /*std::cerr << "[CGI] ENV prepared:\n";
    std::cerr << "  SCRIPT_FILENAME=" << env.scriptFilename << "\n";
    std::cerr << "  SCRIPT_NAME=" << env.scriptName << "\n";
    std::cerr << "  SERVER_PORT=" << env.serverPort << "\n";
    std::cerr << "  GATEWAY_INTERFACE=" << env.gatewayInterface << "\n";
    std::cerr << "  SERVER_NAME=" << env.serverName << "\n";
    std::cerr << "  SERVER_PROTOCOL=" << env.serverProtocol << "\n";
    std::cerr << "  REQUEST_METHOD=" << env.requestMethod << "\n";
    std::cerr << "  REQUEST_URI=" << env.requestUri << "\n";
    std::cerr << "  QUERY_STRING=" << env.queryString << "\n";
    std::cerr << "  CONTENT_LENGTH=" << env.contentLength << "\n";
    std::cerr << "  CONTENT_TYPE=" << env.contentType << "\n";
    std::cerr << "  PATH_INFO=" << env.pathInfo << "\n";
    std::cerr << "--- HTTP_ headers ---\n" << std::endl;*/
    for (std::map<std::string,std::string>::const_iterator it = env.httpHeaders.begin();
             it != env.httpHeaders.end(); ++it)
        {
            //std::cerr << it->first.c_str() << "=" << it->second.c_str() << std::endl;
        }
    return env;
}

char** buildENVP(const CGIEnv &env)
{
    std::vector<std::string> vars;

    vars.push_back("REQUEST_METHOD=" + env.requestMethod);
    vars.push_back("REQUEST_URI=" + env.requestUri);
    vars.push_back("QUERY_STRING=" + env.queryString);
    vars.push_back("CONTENT_LENGTH=" + env.contentLength);
    vars.push_back("CONTENT_TYPE=" + env.contentType);
    vars.push_back("SERVER_PROTOCOL=" + env.serverProtocol);
    vars.push_back("SCRIPT_FILENAME=" + env.scriptFilename);
    vars.push_back("SCRIPT_NAME=" + env.scriptName);
    vars.push_back("SERVER_NAME=" + env.serverName);
    vars.push_back("SERVER_PORT=" + env.serverPort);
    vars.push_back("GATEWAY_INTERFACE=" + env.gatewayInterface);
    vars.push_back("PATH_INFO=" + env.pathInfo);
    vars.push_back("PYTHONUNBUFFERED=1");

    for (std::map<std::string, std::string>::const_iterator it = env.httpHeaders.begin();
         it != env.httpHeaders.end(); ++it)
        vars.push_back(it->first + "=" + it->second);

    char **envp = new char*[vars.size() + 1];
    for (size_t i = 0; i < vars.size(); i++)
        envp[i] = strdup(vars[i].c_str());
    envp[vars.size()] = NULL;

    return envp;
}

void runCGIChild(const LocationConfig &loc, const std::string &fullPath,
                 int inPipe[2], int outPipe[2], char** envp)
{
   // std::cerr << "[CGI CHILD] start, fullPath=" << fullPath << "\n";
    close(inPipe[1]);
    close(outPipe[0]);
    if (dup2(inPipe[0], STDIN_FILENO) == -1) {
      //  std::cerr << "[CGI CHILD] dup2(stdin) failed: " << strerror(errno) << "\n";
        _exit(1);
    }
    if (dup2(outPipe[1], STDOUT_FILENO) == -1) {
       // std::cerr << "[CGI CHILD] dup2(stdout) failed: " << strerror(errno) << "\n";
        _exit(1);
    }
    close(inPipe[0]);
    close(outPipe[1]);
    std::string ext = getExtension(fullPath);
  //  std::cerr << "[CGI CHILD] ext=" << ext << "\n";
    std::map<std::string, std::string>::const_iterator it = loc.cgi.find(ext);
    if (it == loc.cgi.end()) {
       // std::cerr << "[CGI CHILD] no interpreter for ext\n";
        _exit(1);
    }
    const char *interp = it->second.c_str();
  //  std::cerr << "[CGI CHILD] interp=" << interp << "\n";
    std::string scriptDir  = ".";
    std::string scriptFile = fullPath;
    size_t lastSlash = fullPath.rfind('/');
    if (lastSlash != std::string::npos) {
        scriptDir  = fullPath.substr(0, lastSlash);
        scriptFile = fullPath.substr(lastSlash + 1);
    }
    if (chdir(scriptDir.c_str()) == -1) {
     //   std::cerr << "[CGI CHILD] chdir failed: " << strerror(errno) << "\n";
        _exit(1);
    }
  //  std::cerr << "[CGI CHILD] chdir to " << scriptDir << ", script=" << scriptFile << "\n";

    char *argv[3];
    argv[0] = const_cast<char*>(interp);
    argv[1] = const_cast<char*>(scriptFile.c_str());
    argv[2] = NULL;

    //int flags = fcntl(STDIN_FILENO, F_GETFL);
    //std::cerr << "[CGI CHILD] stdin flags=" << flags << "\n";
    //flags = fcntl(STDOUT_FILENO, F_GETFL);
    //std::cerr << "[CGI CHILD] stdout flags=" << flags << "\n";
 //   std::cerr << "[CGI CHILD] execve()...\n";
    //std::cerr << "[CGI CHILD] about to execve\n";
    /*for (int i = 0; envp[i]; i++)
    {
        int fd = open("/tmp/cgi_env_debug.log",
                    O_WRONLY | O_CREAT | O_APPEND,
                    0644);

        if (fd != -1) {
            write(fd, "=== CGI CHILD ENV DEBUG ===\n", 29);

            int i = 0;
            while (envp[i]) {
                write(fd, envp[i], strlen(envp[i]));
                write(fd, "\n", 1);
                i++;
            }

            write(fd, "============================\n\n", 30);
            close(fd);
        }
    }*/
    //std::cerr << "[CGI CHILD] EXECVE PERFORMED WITH : " << interp << " CGI ARGS : " << std::endl;
    /*for (int i = 0; argv[i]; i++)
    {
        std::cerr << " " << argv[i];
    }*/
    //std::cerr << " ENVP DEBUG SEEN ABOVE" << std::endl;
    execve(interp, argv, envp);
    //std::cerr << "[CGI CHILD] execve FAILED errno=" << errno
          //<< " (" << strerror(errno) << ")\n";
    //std::cerr << "[CGI CHILD] execve FAILED: " << strerror(errno) << "\n";
    _exit(1);
}


HttpResponse parseCGIOutput(const std::string &raw)
{
    HttpResponse res;

    size_t pos = raw.find("\r\n\r\n");
    size_t sepLen = 4;

    //std::cerr << "[DEBUG] CGI RAW OUTPUT : " << raw << std::endl;

    if (pos == std::string::npos) {
        pos = raw.find("\n\n");
        sepLen = 2;
    }

    if (pos == std::string::npos) {
        pos = raw.find("\r\r");
        sepLen = 2;
    }

    if (pos == std::string::npos) {
        res.statusCode = 200;
        res.statusMessage = "OK";
        res.headers["Content-Type"] = "text/plain";
        res.body = raw;
        res.headers["Content-Length"] = toString(res.body.size());
        return res;
    }

    std::string headerPart = raw.substr(0, pos);
    std::string bodyPart   = raw.substr(pos + sepLen);

    res.body = bodyPart;

    std::istringstream stream(headerPart);
    std::string line;

    while (std::getline(stream, line))
    {
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);

        size_t colon = line.find(':');
        if (colon == std::string::npos)
            continue;

        std::string key   = line.substr(0, colon);
        std::string value = line.substr(colon + 1);

        if (!value.empty() && value[0] == ' ')
            value.erase(0, 1);

        if (key == "Status")
        {
            std::istringstream ss(value);
            ss >> res.statusCode;
            std::getline(ss, res.statusMessage);

            if (!res.statusMessage.empty() && res.statusMessage[0] == ' ')
                res.statusMessage.erase(0, 1);

            continue;
        }

        res.headers[key] = value;
    }

    if (res.statusCode < 200 || res.statusCode > 599) {
        res.statusCode = 200;
        res.statusMessage = "OK";
    }

    if (res.headers.find("Content-Length") == res.headers.end())
        res.headers["Content-Length"] = toString(res.body.size());

    return res;
}

CGIProcess launchCGI(const HttpRequest &req,
                     const ServerConfig &server,
                     const LocationConfig &loc,
                     const std::string &fullPath)
{
    //std::cerr << "\n[CGI] launchCGI() fullPath=" << fullPath << "\n";

    CGIEnv env = buildCGIEnv(req, server, fullPath);
    char** envp = buildENVP(env);

    int inPipe[2];
    int outPipe[2];

    if (pipe(inPipe) == -1 || pipe(outPipe) == -1) {
        throw std::runtime_error("pipe() failed");
    }

    setNonBlocking(inPipe[1]);
    setNonBlocking(outPipe[0]);

    pid_t pid = fork();
    if (pid < 0) {
        throw std::runtime_error("fork() failed");
    }
    if (pid == 0) {
        runCGIChild(loc, fullPath, inPipe, outPipe, envp);
    }

    //std::cerr << "[CGI] fork OK, pid=" << pid << "\n";

    close(inPipe[0]);
    close(outPipe[1]);

    CGIProcess cgi;
    cgi.pid         = pid;
    cgi.inFd        = inPipe[1];
    cgi.outFd       = outPipe[0];
    cgi.inputBuffer = req.body;
    cgi.inputOffset = 0;
    cgi.inputDone   = req.body.empty();
    cgi.outputDone  = false;
    cgi.startTime   = time(NULL);

    /*std::cerr << "[CGI] launchCGI done:"
              << " inputDone=" << cgi.inputDone
              << " body_size=" << cgi.inputBuffer.size()
              << " inFd=" << cgi.inFd
              << " outFd=" << cgi.outFd
              << "\n";*/

    //std::cerr << "[CGI PARENT] child pid=" << cgi.pid << "\n";
    return cgi;
}
