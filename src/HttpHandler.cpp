/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpHandler.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lylrandr <lylrandr@student.42lausanne.ch>  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/29 18:18:11 by lylrandr          #+#    #+#             */
/*   Updated: 2026/05/14 13:20:48 by lylrandr         ###   ########.fr       */
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

std::string	buildPath(HttpRequest const &req, LocationConfig const &loc){
	(void)loc;
	(void)req;
}

HttpResponse	handleGet(HttpRequest const &request, LocationConfig const &location, std::string path){
	HttpResponse	response;
	struct stat		fileInfo;

	if (stat(path.c_str(), &fileInfo)){
		response.statusCode = 404;
		response.statusMessage = "path not found";
		return(response);
	}
	if (S_ISDIR(fileInfo.st_mode))
		return;
		//listing
	else if (S_ISREG(fileInfo.st_mode)){
		if (access(path.c_str(), fileInfo.st_mode)){
			response.statusMessage = "no access to file";
			response.statusCode = 404;
			return (response);
		}
		
	}
	return (response);
}


HttpResponse	execute(HttpRequest const &req, LocationConfig const &loc){
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
	path = buildPath(req, loc);
	if (req.method == "GET")
		return (handleGet(req, loc, path));
	else if (req.method == "POST")
		return (handlePost(req, loc, path));
	else if (req.method == "DELETE")
		return (handleDelete(req, loc, path));
}
