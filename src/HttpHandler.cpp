/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpHandler.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lylrandr <lylrandr@student.42lausanne.ch>  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/29 18:18:11 by lylrandr          #+#    #+#             */
/*   Updated: 2026/06/30 13:10:06 by lylrandr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HttpHandler.hpp"
#include <sstream>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "CGI.hpp"

#include <sys/stat.h>
#include <string>
#include <vector>

static std::string stripTrailingSlash(const std::string &s)
{
    if (s.size() > 1 && s[s.size() - 1] == '/')
        return s.substr(0, s.size() - 1);
    return s;
}

static std::string ensureLeadingSlash(const std::string &s)
{
    if (s.empty() || s[0] != '/')
        return "/" + s;
    return s;
}

static std::string lastComponent(const std::string &uri)
{
    size_t pos = uri.rfind('/');
    if (pos == std::string::npos || pos == 0)
        return uri.empty() ? "/" : uri;
    return uri.substr(pos);
}

static bool endsWith(const std::string &str, const std::string &suffix)
{
    if (suffix.empty() || str.size() < suffix.size())
        return false;
    return str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

static std::string probe(const std::string &path, const std::string &index)
{
    struct stat st;
    if (stat(path.c_str(), &st) != 0)
        return "";

    if (S_ISDIR(st.st_mode))
    {
        if (index.empty())
            return "";
        std::string idx = stripTrailingSlash(path) + "/" + index;
        struct stat ist;
        if (stat(idx.c_str(), &ist) == 0 && !S_ISDIR(ist.st_mode))
            return idx;

        return "";
    }
    return path;
}

static std::string probeDir(const std::string &path)
{
    struct stat st;
    if (stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode))
        return path;
    return "";
}

LocationConfig route(const HttpRequest &req, const ServerConfig &config)
{
    if (config.locations.empty())
        return LocationConfig();
    std::string uri = stripTrailingSlash(req.uri);
    if (uri.empty()) uri = "/";

    const LocationConfig *best = NULL;
    for (size_t i = 0; i < config.locations.size(); i++)
    {
        const LocationConfig &loc = config.locations[i];
        std::string locPath = stripTrailingSlash(loc.path);
        if (locPath.empty()) locPath = "/";

        bool matches = false;
        if (uri == locPath)
            matches = true;
        else if (uri.compare(0, locPath.size(), locPath) == 0)
        {
            if (locPath == "/" || uri[locPath.size()] == '/')
                matches = true;
        }

        if (matches)
        {
            if (!best || locPath.size() > stripTrailingSlash(best->path).size())
                best = &loc;
        }
    }
    std::string last = lastComponent(uri);
    if (last != "/" && last != uri)
    {
        for (size_t i = 0; i < config.locations.size(); i++)
        {
            const LocationConfig &loc = config.locations[i];
            if (stripTrailingSlash(loc.path) == last)
                return loc;
        }
    }
    if (!best)
        return config.locations[0];
    return *best;
}

std::string resolvePath(const HttpRequest &req,
                        const LocationConfig &loc,
                        const ServerConfig &server)
{
    std::string uri = req.uri;
    size_t q = uri.find('?');
    if (q != std::string::npos)
        uri = uri.substr(0, q);
    bool uriHadTrailingSlash = (!uri.empty() && uri[uri.size() - 1] == '/');
    uri = stripTrailingSlash(uri);
    if (uri.empty()) uri = "/";
    (void)uriHadTrailingSlash;
    std::string serverRoot = stripTrailingSlash(server.root);
    std::string locRoot    = stripTrailingSlash(loc.root);
    if (locRoot.empty())
        locRoot = serverRoot;
    std::string locPath = stripTrailingSlash(loc.path);
    if (locPath.empty()) locPath = "/";

    std::string remainder;
    if (uri == locPath)
        remainder = "/";
    else if (locPath != "/" && uri.compare(0, locPath.size(), locPath) == 0)
        remainder = ensureLeadingSlash(uri.substr(locPath.size()));
    else
        remainder = ensureLeadingSlash(uri);
    std::string locLastSeg = lastComponent(locPath);
    bool patternA = (locLastSeg != "/" && endsWith(locRoot, locLastSeg));
    std::vector<std::string> candidates;
    if (patternA)
    {
        candidates.push_back(locRoot + remainder);
        candidates.push_back(locRoot + uri);
    }
    else
        candidates.push_back(locRoot + uri);
    if (serverRoot != locRoot)
    {
        candidates.push_back(serverRoot + uri);
        candidates.push_back(serverRoot + remainder);
    }
    std::vector<std::string> unique;
    for (size_t i = 0; i < candidates.size(); i++)
    {
        bool dup = false;
        for (size_t j = 0; j < unique.size(); j++)
            if (unique[j] == candidates[i]) { dup = true; break; }
        if (!dup)
            unique.push_back(candidates[i]);
    }
    for (size_t i = 0; i < unique.size(); i++)
    {
        std::string result = probe(unique[i], loc.index);
        if (!result.empty())
            return result;
    }
    if (!patternA && remainder != "/")
    {
        std::string nopFallback = locRoot + remainder;
        bool alreadyTried = false;
        for (size_t i = 0; i < unique.size(); i++)
            if (unique[i] == nopFallback) { alreadyTried = true; break; }
        if (!alreadyTried)
        {
            std::string result = probe(nopFallback, loc.index);
            if (!result.empty())
                return result;
        }
        if (serverRoot != locRoot)
        {
            std::string srvFallback = serverRoot + remainder;
            bool srvTried = false;
            for (size_t i = 0; i < unique.size(); i++)
                if (unique[i] == srvFallback) { srvTried = true; break; }

            if (!srvTried)
            {
                std::string result = probe(srvFallback, loc.index);
                if (!result.empty())
                    return result;
            }
        }
    }
    std::string primaryCandidate = patternA ? (locRoot + remainder) : (locRoot + uri);
    {
        std::string result = probeDir(primaryCandidate);
        if (!result.empty())
            return result;
    }
    return "";
}

HttpRequest parseRequest(const std::string &buffer,
                         size_t bodyOffset,
                         size_t bodyLength)
{
    HttpRequest req;
    std::string key, value, line;

    std::istringstream stream(buffer);
    stream >> req.method >> req.uri >> req.version;
    std::getline(stream, line);

    while (std::getline(stream, line)) {
        if (line == "\r" || line.empty())
            break;

        size_t pos = line.find(':');
        if (pos == std::string::npos)
            continue;

        key = line.substr(0, pos);
        value = line.substr(pos + 1);
        while (!value.empty() && value[0] == ' ')
            value.erase(0, 1);
        if (!value.empty() && value[value.size() - 1] == '\r')
            value.erase(value.size() - 1);

        req.headers[key] = value;
    }
    if (bodyLength > 0 && bodyOffset + bodyLength <= buffer.size())
        req.body = buffer.substr(bodyOffset, bodyLength);

    return req;
}

std::string getMimeType(const std::string &path)
{
    size_t dot = path.find_last_of('.');
    if (dot == std::string::npos)
        return "application/octet-stream";

    std::string ext = path.substr(dot + 1);
	if (ext == "html") return "text/html";
	if (ext == "css")  return "text/css";
	if (ext == "js")   return "application/javascript";
	if (ext == "png")  return "image/png";
	if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
	if (ext == "gif")  return "image/gif";
	if (ext == "ico")  return "image/x-icon";
	if (ext == "txt")  return "text/plain";

    return "application/octet-stream";
}

HttpResponse serveFile(const std::string &path, const ServerConfig &config)
{
    HttpResponse response;
    int fd = open(path.c_str(), O_RDONLY);

    if (fd < 0)
		return (buildError(404, "Not found", config));

    char buf[4096];
    ssize_t bytes;

    while ((bytes = read(fd, buf, sizeof(buf))) > 0)
        response.body.append(buf, bytes);
    if (bytes < 0)
    {
        close (fd);
        return buildError(500, "Internal Server Error", config);
    }
    if (bytes >= 0)
    {
        close(fd);
        std::ostringstream oss;
        oss << response.body.size();
        response.statusCode = 200;
        response.statusMessage = "OK";
        response.headers["Content-Type"] = getMimeType(path);
        response.headers["Content-Length"] = oss.str();
    }
    return response;
}

HttpResponse execute(const HttpRequest &req,
                     const LocationConfig &loc,
                     const ServerConfig &server)
{
    HttpResponse response;

    bool allowed = false;
    for (size_t i = 0; i < loc.methods.size(); i++) {
        if (loc.methods[i] == req.method)
            allowed = true;
    }
    if (!allowed)
        return (buildError(405, "Not allowed", server));
    std::string path = resolvePath(req, loc, server);
    if (req.method == "GET")
        return handleGet(loc, path, server);
    else if (req.method == "POST")
        return handlePost(req, loc, server, path);
    else if (req.method == "DELETE")
        return handleDelete(path, server);
    return makeUploadResponse(100, "Unsupported request type", "");
}
