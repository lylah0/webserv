/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerConfig.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cjauregu <cjauregu@student.42lausanne.c    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/09 17:23:56 by lylrandr          #+#    #+#             */
/*   Updated: 2026/04/29 12:15:38 by cjauregu         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVERCONFIG_HPP
#define SERVERCONFIG_HPP

#include <string>
#include <vector>
#include <map>
#include <iostream>

struct LocationConfig {
    std::string              path;
    std::vector<std::string> methods;
    std::string              root;
    std::string              index;
    bool                     autoindex;
    std::string              upload_store;
    bool                     upload_enabled;
    std::string              redirect;
    std::map<std::string, std::string> cgi;
};

struct ServerConfig {
    std::string server_name;
    int listen;
    std::string host;
    std::string root;
    std::string index;
    size_t client_max_body_size;
    std::string error_page;
    std::vector<LocationConfig> locations;
};

#endif
