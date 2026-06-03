/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerConfig.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lylrandr <lylrandr@student.42lausanne.ch>  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/09 17:23:56 by lylrandr          #+#    #+#             */
/*   Updated: 2026/05/27 17:21:14 by lylrandr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVERCONFIG_HPP
#define SERVERCONFIG_HPP

# include <string>
# include <vector>
# include "LocationConfig.hpp"

struct ServerConfig {
	int							listen;
	size_t						client_max_body_size;
	std::string					server_name;
	std::string					host;
	std::string					root;
	std::string					index;
	std::map<int, std::string>	error_page;
	std::vector<LocationConfig>	locations;
};

#endif
