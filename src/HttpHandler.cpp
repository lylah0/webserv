/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpHandler.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cjauregu <cjauregu@student.42lausanne.c    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/29 18:18:11 by lylrandr          #+#    #+#             */
/*   Updated: 2026/05/12 22:01:02 by cjauregu         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HttpHandler.hpp"

HttpRequest	parseRequest(std::string const &buffer){
	std::string			body;
	std::string			key;
	std::string			value;
	std::string			line;
	HttpRequest			req;
	std::istringstream	stream(buffer);

	stream >> req.method >> req.uri >> req.version;
	std::getline(stream, line);
	while (std::getline(stream,line)){
		if (line == "\r" || line.empty())
			break;
		key = line.substr(0, line.find(':'));
		value = line.substr(line.find(':') + 2);
		if (!value.empty() && value[value.size() - 1] == '\r')
			value.erase(value.size() - 1);
		req.headers[key] = value;
	}
	std::getline(stream, body, '\0');
	req.body = body;
	return (req);
}

LocationConfig	route(HttpRequest const &req, ServerConfig const &config){
	LocationConfig	loc;

	for (size_t i = 0; i < config.locations.size(); i++){
		if (req.uri.find(config.locations[i].path) == 0){
			if (loc.path.empty() || config.locations[i].path.size() > loc.path.size())
					loc = config.locations[i];
			}
		}
	return(loc);
}

std::string resolvePath(const HttpRequest &req, ServerConfig const &server, const LocationConfig &loc)
{
	std::string root;
	if (!loc.root.empty())
		root = loc.root;
	else
		root = server.root;
	std::string localUri = req.uri;
	if (!loc.path.empty() && loc.path != "/")
	{
		if (req.uri.find(loc.path) == 0)
			localUri = req.uri.substr(loc.path.length());
	}
	if (localUri.empty() || localUri == "/")
        localUri = "/" + (loc.index.empty() ? server.index : loc.index);
	return root + localUri;
}

std::string getMimeType(const std::string& path)
{
	size_t dot = path.find_last_of('.');
	if (dot == std::string::npos)
		return "application/octet-stream";
	std::string ext = path.substr(dot + 1);
	std::cout << "Extension : " << ext << std::endl;

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

std::string serveFile(const std::string &fullPath)
{
	int fd = open(fullPath.c_str(), O_RDONLY);
	if (fd < 0) {
        // File not found, return 404
        std::string body = "<h1>404 Not Found</h1>";
        std::ostringstream out;
        out << "HTTP/1.1 404 Not Found\r\n";
        out << "Content-Length: " << body.size() << "\r\n";
        out << "Content-Type: text/html\r\n";
        out << "Connection: close\r\n\r\n";
        out << body;
        return out.str();
    }
	std::string body;
	char buf[4096];
	ssize_t bytes;

	while ((bytes = read(fd, buf, sizeof(buf))) > 0)
		body.append(buf, bytes);
	close(fd);
	std::ostringstream out;
	std::string mime = getMimeType(fullPath);
	std::cout << "Mime Type " << mime << std::endl;
	out << "HTTP/1.1 200 OK\r\n";
	out << "Content-Length: " << body.size() << "\r\n";
	out << "Content-Type: " << mime << "\r\n";
	out << "Connection: close\r\n\r\n";
    out << body;

	return out.str();
}

// HttpResponse	execute(HttpRequest const &req, LocationConfig const &loc){
// 	(void)req;
// 	(void)loc;
// }
