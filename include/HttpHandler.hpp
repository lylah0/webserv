/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpHandler.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lylrandr <lylrandr@student.42lausanne.ch>  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/29 18:17:37 by lylrandr          #+#    #+#             */
/*   Updated: 2026/05/14 13:14:31 by lylrandr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTPHANDLER_HPP
#define HTTPHANDLER_HPP

# include <string>
# include <iostream>
# include <sstream>
#include <sys/stat.h>
#include <unistd.h>
# include "HttpRequest.hpp"
# include "LocationConfig.hpp"
# include "HttpResponse.hpp"
# include "ServerConfig.hpp"

HttpRequest		parseRequest(std::string const &buffer);
HttpResponse	execute(HttpRequest const &req, LocationConfig const &loc);
LocationConfig	route(HttpRequest const &req, ServerConfig const &config);
HttpResponse	handleGet(HttpRequest const &request, LocationConfig const &location, std::string path);
HttpResponse	handlePost(HttpRequest const &request, LocationConfig const &location, std::string path);
HttpResponse	handleDelete(HttpRequest const &request, LocationConfig const &location, std::string path);

#endif
