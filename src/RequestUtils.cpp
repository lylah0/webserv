/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   RequestUtils.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lylrandr <lylrandr@student.42lausanne.ch>  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/22 16:31:11 by lylrandr          #+#    #+#             */
/*   Updated: 2026/05/22 16:31:55 by lylrandr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HttpHandler.hpp"

HttpResponse	handleGet(LocationConfig const &location, std::string path){
	HttpResponse		response;
	struct stat			fileInfo;
	std::string			name;
	DIR					*dir;
	dirent				*entry;
	std::ostringstream	oss;

	if (stat(path.c_str(), &fileInfo) < 0){
		response.statusCode = 404;
		response.statusMessage = "Not found";
		return(response);
	}
	if (S_ISDIR(fileInfo.st_mode)){
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
