/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   SocketUtils.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lylrandr <lylrandr@student.42lausanne.ch>  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/09 15:44:34 by lylrandr          #+#    #+#             */
/*   Updated: 2026/04/09 17:09:51 by lylrandr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "SocketUtils.hpp"

int	setNonBlocking(int fd){
	int flags;
	flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1)
		return (-1);
	flags = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
	if (flags == -1)
		return (-1);
	return (0);
}
