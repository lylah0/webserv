/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerConfig.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lylrandr <lylrandr@student.42lausanne.ch>  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/09 17:23:56 by lylrandr          #+#    #+#             */
/*   Updated: 2026/04/09 17:24:57 by lylrandr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVERCONFIG_HPP
#define SERVERCONFIG_HPP

#include <string>
#include <vector>

struct ServerConfig {
	std::string	server_name;
	int			listen;
	std::string	host;
	std::string	root;
	std::string	index;
	size_t		client_max_body_size;
	std::string	error_page;
};

#endif
