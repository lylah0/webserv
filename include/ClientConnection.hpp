/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ClientConnection.hpp                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lylrandr <lylrandr@student.42lausanne.ch>  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/13 18:27:14 by lylrandr          #+#    #+#             */
/*   Updated: 2026/04/27 16:26:23 by lylrandr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENTCONNECTION_HPP
#define CLIENTCONNECTION_HPP

# include <string>
# include <map>
# include <unistd.h>
# include <sys/socket.h>

class	ClientConnection{
	private :
		int			_fd;
		std::string	_readBuffer;
		std::string	_writeBuffer;
		size_t		_writeOffset;

		ClientConnection(ClientConnection const &src);
		ClientConnection&	operator=(ClientConnection const &rhs);
	public :
		ClientConnection(int fd);
		~ClientConnection();

		std::string				getBuffer() const;
		size_t					getOffset() const;
		int						getFd() const;
		std::string const&		getReadBuffer() const;
		bool					handleRead();
		bool					handleWrite();
		void					prepResponse(std::string const &response);
};

#endif
