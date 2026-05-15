/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpHandler.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lylrandr <lylrandr@student.42lausanne.ch>  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/29 18:18:11 by lylrandr          #+#    #+#             */
/*   Updated: 2026/05/15 20:39:57 by lylrandr         ###   ########.fr       */
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

HttpResponse	handleGet(LocationConfig const &location, std::string path){
	HttpResponse	response;
	struct stat		fileInfo;
	std::string		name;
	DIR				*dir;
	dirent			*entry;

	if (stat(path.c_str(), &fileInfo) < 0){
		response.statusCode = 404;
		response.statusMessage = "Not found";
		return(response);
	}
	if (S_ISDIR(fileInfo.st_mode)){
		std::cout << "autoindex: " << location.autoindex << std::endl;
		if (location.autoindex){
			dir = opendir(path.c_str());
			std::cout << "opendir result: " << (dir == NULL ? "NULL" : "OK") << std::endl;
			if (dir == NULL){
				response.statusCode = 500;
				response.statusMessage = "Internal server error";
				return(response);
			}
			response.body = "<html><body><h1>Index of :" + path + "</h1><ul>";
			while ((entry = readdir(dir)) != NULL){
				name = entry->d_name;
				response.body += "<li><a href=\"" + name + "\">" + name + "</a></li>";
			}
			response.body += "</ul></body></html>";
			closedir(dir);
			response.statusCode = 200;
			response.statusMessage = "OK";
			return(response);
		}
		else{
			response.statusCode = 403;
			response.statusMessage = "Forbidden";
			return (response);
		}
	}
	else if (S_ISREG(fileInfo.st_mode)){
		if (access(path.c_str(), R_OK) < 0){
			response.statusMessage = "Forbidden";
			response.statusCode = 403;
			return (response);
		}
		return (serveFile(path));
	}
	return (response);
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

HttpResponse	serveFile(std::string const &path){
	HttpResponse	response;
	int				fd = open(path.c_str(), O_RDONLY);

	std::cout << "serveFile: " << path << std::endl;
	if (fd < 0)
	{
		response.statusCode = 404;
		response.statusMessage = "Not Found";
		response.body = "<html><body><h1>404 Not Found</h1></body></html>";
		response.headers["Content-Type"] = "text/html";
		response.headers["Content-Length"] = "47";
		return (response);
	}
	char	buf[4096];
	ssize_t bytes;
	while ((bytes = read(fd, buf, sizeof(buf))) > 0)
		response.body.append(buf, bytes);
	close(fd);

	std::ostringstream oss;
	oss << response.body.size();

	response.statusCode    = 200;
	response.statusMessage = "OK";
	response.headers["Content-Type"]   = getMimeType(path);
	response.headers["Content-Length"] = oss.str();
	return (response);
}

HttpResponse	execute(HttpRequest const &req, LocationConfig const &loc, ServerConfig const &server){
	bool			allowed;
	std::string		path;
	HttpResponse	response;

	allowed = false;
	for (size_t i = 0; i < loc.methods.size(); i++){
		if (loc.methods[i] == req.method)
			allowed = true;
	}
	if (!allowed){
		response.statusCode = 405;
		response.statusMessage = "method not allowed";
		return (response);
	}
	path = resolvePath(req, server, loc);
	if (req.method == "GET")
		return (handleGet(loc, path));
	else if (req.method == "POST")
		return(response);

		// return (handlePost(req, loc, path));
	else if (req.method == "DELETE")
		return(response);

	return (response);
	// return (handleDelete(req, loc, path));
}
