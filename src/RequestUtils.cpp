/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestUtils.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lylrandr <lylrandr@student.42lausanne.ch>  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/22 16:31:11 by lylrandr          #+#    #+#             */
/*   Updated: 2026/05/27 17:21:09 by lylrandr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HttpHandler.hpp"

HttpResponse	buildError(int code, std::string const &message, ServerConfig const &config){
	HttpResponse		response;
	std::ostringstream	oss;

	if (config.error_page.empty()){
		oss << code;
		response.body = "<html><body><h1>Error :" + oss.str() + message + "</h1><ul>";
	}
	else{
		response = serveFile(config.root);
	}
	return (response);
}

HttpResponse	isDir(LocationConfig const &location, std::string path, HttpResponse response){
	std::ostringstream	oss;
	std::string			name;
	DIR					*dir;
	dirent				*entry;

	if (location.autoindex){
	dir = opendir(path.c_str());
	if (dir == NULL){
		response.statusCode = 500;
		response.statusMessage = "Internal server error";
		return(response);
	}
	response.body = "<html><body><h1>Index of :" + path + "</h1><ul>";
	while ((entry = readdir(dir)) != NULL){
		name = entry->d_name;
		if (name == "." || name == "..")
			continue;
		response.body += "<li><a href=\"" + name + "\">" + name + "</a></li>";
	}
	response.body += "</ul></body></html>";
	oss << response.body.size();
	response.headers["Content-Type"] = "text/html";
	response.headers["Content-Length"] = oss.str();
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

HttpResponse	handleGet(LocationConfig const &location, std::string path){
	HttpResponse		response;
	struct stat			fileInfo;
	ServerConfig		config;

	if (stat(path.c_str(), &fileInfo) < 0){
		return (buildError(404, "Not found", config));
	}
	if (S_ISDIR(fileInfo.st_mode))
		return (isDir(location, path, response));
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

//verifier max body size --> 413
HttpResponse	handlePost(HttpRequest const &request, LocationConfig const &location){
	HttpResponse	response;
	std::string		contentType;
	std::string		boundary;
	std::string		filename;
	std::string		content;
	std::string		filepath;
	size_t			pos;
	int				fd;

	contentType = request.headers.at("Content-Type");
	pos = contentType.find("boundary=");
	boundary = request.body.substr(pos + 9);
	pos = request.body.find("filename=\"");
	filename = request.body.substr(pos + 10);
	filename = filename.substr(0, filename.find("\""));
	pos = request.body.find("Content-Type:");
	pos =  request.body.find("\r\n\r\n", pos);
	content = request.body.substr(pos);
	content = content.substr(0, content.find("--"));
	if (content.size() >= 2 && content[0] == '\r' && content[1] == '\n')
		content = content.substr(2);
	if (content.size() >= 2 && content[content.size()-2] == '\r' && content[content.size()-1] == '\n')
		content = content.substr(0, content.size() - 2);
	filepath = location.upload_store + '/' + filename;
	fd = open(filepath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd < 0){
		response.statusCode = 500;
		response.statusMessage = "Internal Server Error";
		return (response);
	}
	write(fd, content.c_str(), content.size());
	close(fd);
	response.statusCode = 201;
	response.statusMessage = "Created";
	return (response);
}
