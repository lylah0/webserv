/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpHandler.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lylrandr <lylrandr@student.42lausanne.ch>  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/29 18:18:11 by lylrandr          #+#    #+#             */
/*   Updated: 2026/06/09 00:51:13 by lylrandr         ###   ########.fr       */
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
	if (!stream.good())
		return (req);
	while (std::getline(stream,line)){
		std::cout << "in getline loop" << std::endl;
		if (line == "\r" || line.empty())
			break;
		size_t colon = line.find(':');
		if (colon == std::string::npos)
			continue;
		key = line.substr(0, colon);
		value = line.substr(colon + 2);
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

HttpResponse	serveFile(std::string const &path, ServerConfig const &config){
	std::ostringstream	oss;
	HttpResponse		response;
	int					fd = open(path.c_str(), O_RDONLY);

	if (fd < 0)
		return (buildError(404, "Not found", config));
	char	buf[4096];
	ssize_t bytes;
	while ((bytes = read(fd, buf, sizeof(buf))) > 0)
		response.body.append(buf, bytes);
	close(fd);
	oss << response.body.size();
	response.statusCode    = 200;
	response.statusMessage = "OK";
	response.headers["Content-Type"]   = getMimeType(path);
	response.headers["Content-Length"] = oss.str();
	return (response);
}

HttpResponse	execute(HttpRequest const &req, LocationConfig const &loc, ServerConfig const &config){
	bool			allowed;
	std::string		path;
	HttpResponse	response;

	allowed = false;
	for (size_t i = 0; i < loc.methods.size(); i++){
		if (loc.methods[i] == req.method)
			allowed = true;
	}
	if (!allowed)
		return (buildError(405, "Not allowed", config));
	path = resolvePath(req, config, loc);
	if (req.method == "GET")
		return (handleGet(loc, path, config));
	else if (req.method == "POST")
		return (handlePost(req, loc, config));
	else if (req.method == "DELETE")
		return(response);
	return (response);
	// return (handleDelete(req, loc, path));
}
