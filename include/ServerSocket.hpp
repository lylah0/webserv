/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerSocket.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lylrandr <lylrandr@student.42lausanne.ch>  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/09 14:59:52 by lylrandr          #+#    #+#             */
/*   Updated: 2026/05/04 11:01:27 by lylrandr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVERSOCKET_HPP
#define SERVERSOCKET_HPP

# include "ServerConfig.hpp"
# include "Parser.hpp"
# include "SocketUtils.hpp"
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <iostream>

class ServerSocket {
	private :
		int					_fd;
		struct sockaddr_in	_addr;
		ServerSocket(ServerSocket const &src);
		ServerSocket&	operator=(ServerSocket const &rhs);
	public :
		ServerSocket(ServerConfig const &config);
		~ServerSocket();
		int	getFd() const;
		int	acceptClient() const;
};

#endif

