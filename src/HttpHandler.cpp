/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpHandler.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lylrandr <lylrandr@student.42lausanne.ch>  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/29 18:18:11 by lylrandr          #+#    #+#             */
/*   Updated: 2026/06/23 21:40:48 by lylrandr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HttpHandler.hpp"
#include <sstream>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "CGI.hpp"

HttpRequest parseRequest(const std::string &buffer,
                         size_t bodyOffset,
                         size_t bodyLength)
{
    HttpRequest req;
    std::string key, value, line;

    std::istringstream stream(buffer);
    //std::cerr << "[PARSE] First line raw: "
      //        << buffer.substr(0, buffer.find("\r\n")) << "\n";
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

LocationConfig route(const HttpRequest &req, const ServerConfig &config)
{
    const LocationConfig *best = NULL;

    for (size_t i = 0; i < config.locations.size(); i++) {
        const LocationConfig &loc = config.locations[i];
        if (req.uri.compare(0, loc.path.size(), loc.path) == 0) {
            if (!best || loc.path.size() > best->path.size())
                best = &loc;
        }
    }
    if (!best)
        return config.locations[0];
    return *best;
}

std::string resolvePath(const HttpRequest &req, const LocationConfig &loc)
{
    std::string uri = req.uri;

    size_t q = uri.find('?');
    if (q != std::string::npos)
        uri = uri.substr(0, q);

    if (uri.empty() || uri[0] != '/')
        uri = "/" + uri;

    std::string root = loc.root;
    if (!root.empty() && root[root.size() - 1] == '/')
        root.erase(root.size() - 1);

    std::string locPath = loc.path;
    if (locPath.empty())
        locPath = "/";
    if (locPath[0] != '/')
        locPath = "/" + locPath;
    if (locPath.size() > 1 && locPath[locPath.size() - 1] == '/')
        locPath.erase(locPath.size() - 1);

    std::string remainder;

    if (locPath == "/") {
        remainder = uri;
    } else {
        std::string prefix = locPath + "/";
        if (uri.compare(0, prefix.size(), prefix) == 0)
            remainder = uri.substr(locPath.size());
        else
            remainder = uri;
    }

    if (remainder.empty() || remainder[0] != '/')
        remainder = "/" + remainder;

    std::string path = root + remainder;

    struct stat st;
    if (stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
        if (!loc.index.empty()) {
            if (!path.empty() && path[path.size() - 1] != '/')
                path += "/";
            path += loc.index;
        }
    }

    return path;
}

std::string getMimeType(const std::string &path)
{
    size_t dot = path.find_last_of('.');
    if (dot == std::string::npos)
        return "application/octet-stream";

    std::string ext = path.substr(dot + 1);
    //std::cout << "Extension : " << ext << "\n";

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

    close(fd);

    std::ostringstream oss;
    oss << response.body.size();

    response.statusCode = 200;
    response.statusMessage = "OK";
    response.headers["Content-Type"] = getMimeType(path);
    response.headers["Content-Length"] = oss.str();

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
    if (!allowed) {
        return (buildError(405, "Method Not Allowed", server));
    }
    std::string path = resolvePath(req, loc);
    if (req.method == "GET")
        return handleGet(loc, path, server);
    else if (req.method == "POST")
        return handlePost(req, loc, server, path);
    else if (req.method == "DELETE")
        return handleDelete(path, server);
    return makeUploadResponse(100, "Unsupported request type", "");
}
