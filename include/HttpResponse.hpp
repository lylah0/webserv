/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpResponse.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lylrandr <lylrandr@student.42lausanne.ch>  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/13 15:57:48 by lylrandr          #+#    #+#             */
/*   Updated: 2026/06/30 14:13:01 by lylrandr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

# include <string>
# include <map>
# include <dirent.h>

struct	HttpResponse{
	int									statusCode;
	std::string							statusMessage;
	std::map<std::string, std::string>	headers;
	std::string							body;
	bool								isCGIPending;
	bool								isChunkedPending;
};

#endif
