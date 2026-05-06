/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpHandler.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lylrandr <lylrandr@student.42lausanne.ch>  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/29 18:18:11 by lylrandr          #+#    #+#             */
/*   Updated: 2026/05/06 14:13:37 by lylrandr         ###   ########.fr       */
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

// LocationConfig	route(HttpRequest const &req, ServerConfig const &config){
// 	(void)req;
// 	(void)config;
// }

// HttpResponse	execute(HttpRequest const &req, LocationConfig const &loc){
// 	(void)req;
// 	(void)loc;
// }
