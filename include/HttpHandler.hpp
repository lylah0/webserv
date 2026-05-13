/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpHandler.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cjauregu <cjauregu@student.42lausanne.c    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/29 18:17:37 by lylrandr          #+#    #+#             */
/*   Updated: 2026/05/12 21:46:06 by cjauregu         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTPHANDLER_HPP
#define HTTPHANDLER_HPP

# include <string>
# include <iostream>
# include <sstream>
# include <fcntl.h>
# include <unistd.h>
# include "HttpRequest.hpp"
# include "LocationConfig.hpp"
# include "HttpResponse.hpp"
# include "ServerConfig.hpp"

HttpRequest		parseRequest(std::string const &buffer);
HttpResponse	execute(HttpRequest const &req, LocationConfig const &loc);
LocationConfig	route(HttpRequest const &req, ServerConfig const &config);
std::string     resolvePath(const HttpRequest &req, ServerConfig const &server, const LocationConfig &loc);
std::string     serveFile(const std::string &fullPath);

#endif
