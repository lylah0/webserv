#ifndef SCRIPT_HPP
# define SCRIPT_HPP

#include <string>
#include <sstream>
#include <map>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "CGIProcess.hpp"
struct HttpRequest;
struct HttpResponse;
struct ServerConfig;
struct LocationConfig;

struct CGIEnv {
    std::string requestMethod;
    std::string requestUri;
    std::string queryString;
    std::string contentLength;
    std::string contentType;
    std::string serverProtocol;
    std::string scriptFilename;
    std::string scriptName;
    std::string serverName;
    std::string serverPort;
    std::string gatewayInterface;
    std::string pathInfo;
    std::map<std::string, std::string> httpHeaders;
};

const LocationConfig* findExtensionLocation(const ServerConfig& server, const std::string& uri, const std::string& resolvedPath, const std::string& ext);
bool isCGIvalid(const LocationConfig& extLoc, const std::string& ext, const std::string& method);
std::string getExtension(const std::string &path);
std::string CGI_validation_check(const std::string& fullPath);
CGIProcess launchCGI(const HttpRequest &req, const ServerConfig &server, const LocationConfig &loc, const std::string &fullPath);
HttpResponse parseCGIOutput(const std::string &raw);

#endif
