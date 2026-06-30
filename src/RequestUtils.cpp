/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestUtils.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lylrandr <lylrandr@student.42lausanne.ch>  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/22 16:31:11 by lylrandr          #+#    #+#             */
/*   Updated: 2026/06/30 14:03:19 by lylrandr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/HttpHandler.hpp"
#include "CGI.hpp"

HttpResponse buildError(int code, const std::string &message, const ServerConfig &config)
{
    HttpResponse response;
    std::ostringstream oss;

    response.statusCode    = code;
    response.statusMessage = message;
    response.headers["Content-Type"] = "text/html";
    std::map<int, std::string>::const_iterator it = config.error_page.find(code);
    if (it != config.error_page.end())
    {
        std::string fullpath = config.root + it->second;
        int fd = open(fullpath.c_str(), O_RDONLY);

        if (fd >= 0)
        {
            char buf[4096];
            ssize_t n;

            while ((n = read(fd, buf, sizeof(buf))) > 0)
                response.body.append(buf, static_cast<size_t>(n));
            if (n < 0)
                close(fd);
            else if (n == 0)
            {
                close(fd);
                oss << response.body.size();
                response.headers["Content-Length"] = oss.str();
                return response;
            }
        }
    }
    oss << code;
    response.body = "<html><body><h1>" + oss.str() + " " + message + "</h1></body></html>";
    oss.str("");
    oss.clear();
    oss << response.body.size();
    response.headers["Content-Length"] = oss.str();
    return response;
}

bool parseRequestFromBuffer(const std::string &buf, HttpRequest &outReq, size_t &consumed) {
    consumed = 0;

    size_t pos = buf.find("\r\n\r\n");
    if (pos == std::string::npos)
        return false;

    size_t headerEnd = pos + 4;
    std::string headerBlock = buf.substr(0, headerEnd);
    size_t contentLength = 0;
    size_t clPos = headerBlock.find("Content-Length:");
    if (clPos != std::string::npos) {
        clPos += strlen("Content-Length:");
        while (clPos < headerBlock.size() && headerBlock[clPos] == ' ')
            ++clPos;
        contentLength = static_cast<size_t>(std::atoi(headerBlock.c_str() + clPos));
    }

    size_t totalNeeded = headerEnd + contentLength;
    if (buf.size() < totalNeeded)
        return false;
    std::string requestSlice = buf.substr(0, totalNeeded);
    outReq = parseRequest(requestSlice, headerEnd, contentLength);
    consumed = totalNeeded;
    return true;
}

static std::string toString(size_t n) {
    std::stringstream ss;
    ss << n;
    return ss.str();
}

HttpResponse isDir(LocationConfig const &location,
                   std::string path,
                   HttpResponse response, ServerConfig const &config)
{
    std::ostringstream oss;
    std::string        name;
    DIR               *dir;
    dirent            *entry;

    if (location.autoindex) {
        dir = opendir(path.c_str());
        if (dir == NULL) {
			return (buildError(500, "Internal server error", config));
        }
        response.body = "<html><body><h1>Index of: " + path + "</h1><ul>";
        while ((entry = readdir(dir)) != NULL) {
            name = entry->d_name;
            if (name == "." || name == "..")
                continue;
            response.body += "<li><a href=\"" + name + "\">" + name + "</a></li>";
        }
        response.body += "</ul></body></html>";
        closedir(dir);

        response.statusCode = 200;
        response.statusMessage = "OK";
        response.headers["Content-Type"] = "text/html";
        response.headers["Content-Length"] =
            toString(response.body.size());
        return response;
    } else {
		return(buildError(403, "Forbidden", config));
    }
}

HttpResponse handleGet(LocationConfig const &location, std::string path, ServerConfig const &config)
{
    HttpResponse response;
    struct stat  fileInfo;

    if (stat(path.c_str(), &fileInfo) < 0)
		return (buildError(404, "Not found", config));
    if (S_ISDIR(fileInfo.st_mode)) {
        return isDir(location, path, response, config);
    } else if (S_ISREG(fileInfo.st_mode)) {
        if (access(path.c_str(), R_OK) < 0) {
			return(buildError(403, "Forbidden", config));
        }
        return serveFile(path, config);
    }
	return(buildError(404, "Not found", config));
}

HttpResponse handlePost(const HttpRequest& request,
                        const LocationConfig& location,
                        const ServerConfig& server,
                        const std::string& path)
{
    (void)server;
    (void)path;
    if (!location.upload_enabled)
        return buildError(400, "Uploads not enabled for this location", server);
    std::map<std::string, std::string>::const_iterator it =
        request.headers.find("Content-Type");
    if (it == request.headers.end())
        return buildError(400, "Missing Content-Type", server);
    const std::string &contentType = it->second;
    if (contentType.find("multipart/form-data") == 0)
    {
        size_t bpos = contentType.find("boundary=");
        if (bpos == std::string::npos)
            return buildError(400, "Missing boundary in Content-Type", server);
        std::string boundary = contentType.substr(bpos + 9);
        while (!boundary.empty()) {
            char c = boundary[boundary.size() - 1];
            if (c == '\r' || c == '\n' || c == ';' || c == ' ')
                boundary.erase(boundary.size() - 1);
            else
                break;
        }
        boundary = "--" + boundary;
        return parseMultipartAndSave(request.body, boundary, location, server);
    }
    return handleRawPostBody(request, location, server);
}


HttpResponse handleDelete(const std::string& path, const ServerConfig& server)
{
    HttpResponse res;

    struct stat st;
    if (stat(path.c_str(), &st) < 0) {
		return buildError(404, "Not found", server);
    }
	if (S_ISDIR(st.st_mode)) {
		return buildError(403, "Forbidden", server);
	}
    if (access(path.c_str(), W_OK) == -1) {
		return buildError(403, "Forbidden", server);
    }
    if (unlink(path.c_str()) == -1) {
		return buildError(500, "Not found", server);
    }
    return makeUploadResponse(204, "Success", "");
}
