/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   LocationConfig.hpp                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lylrandr <lylrandr@student.42lausanne.ch>  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/27 18:26:25 by lylrandr          #+#    #+#             */
/*   Updated: 2026/04/29 18:16:12 by lylrandr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef LOCATIONCONFIG_HPP
#define LOCATIONCONFIG_HPP

# include <vector>
# include <string>
# include <map>

struct LocationConfig {
	bool								autoindex;
	std::string							path;
	std::string							root;
	std::string							index;
	std::string							redirect;
	std::vector<std::string>			methods;
	std::map<std::string, std::string>	cgi;
};

#endif
