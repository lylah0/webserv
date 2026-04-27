/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ClientConnection.cpp                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lylrandr <lylrandr@student.42lausanne.ch>  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/13 19:15:42 by lylrandr          #+#    #+#             */
/*   Updated: 2026/04/14 16:32:19 by lylrandr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ClientConnection.hpp"

ClientConnection::ClientConnection(int fd) : _fd(fd), _writeOffset(0){}

ClientConnection::~ClientConnection(){
	close(_fd);
}

int	ClientConnection::getFd() const{
	return (_fd);
}

std::string const&	ClientConnection::getReadBuffer() const{
	return (_readBuffer);
}

bool	ClientConnection::handleRead(){
	char	buf[4096];
	ssize_t	bytes;
	bytes = recv(_fd, buf, sizeof(buf), 0);
	if (bytes <= 0)
		return (false);
	_readBuffer.append(buf, bytes);
	return (true);
}

bool	ClientConnection::handleWrite(){
	size_t		rest;
	ssize_t		bytes;
	const char	*data;

	data = _writeBuffer.c_str() + _writeOffset;
	rest = _writeBuffer.size() - _writeOffset;
	bytes = send(_fd, data, rest, 0);
	if (bytes < 0)
		return (false);
	_writeOffset += bytes;
	return (true);
}

void	ClientConnection::prepResponse(std::string const &response){
	_writeBuffer = response;
	_writeOffset = 0;
}
