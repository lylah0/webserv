/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ClientState.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lylrandr <lylrandr@student.42lausanne.ch>  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/13 12:48:54 by lylrandr          #+#    #+#             */
/*   Updated: 2026/04/13 13:28:58 by lylrandr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENTSTATE_HPP
#define CLIENTSTATE_HPP

#include <string>

struct	ClientState{
	std::string	buffer;
	bool		headersComplete;
	bool		requestReady;
	size_t		contentLength;
	ClientState() : headersComplete(false), requestReady(false), contentLength(0){}
};

#endif
