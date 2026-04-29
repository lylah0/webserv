/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequest.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lylrandr <lylrandr@student.42lausanne.ch>  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/13 13:31:53 by lylrandr          #+#    #+#             */
/*   Updated: 2026/04/13 15:56:51 by lylrandr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

# include <string>
# include <map>

struct HttpRequest{
	std::string							method;
	std::string							uri;
	std::string							version;
	std::map<std::string, std::string>	headers;
	std::string							body;
};

#endif
