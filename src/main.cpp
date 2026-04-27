/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lylrandr <lylrandr@student.42lausanne.ch>  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/09 14:54:32 by lylrandr          #+#    #+#             */
/*   Updated: 2026/04/27 02:13:41 by lylrandr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "SocketUtils.hpp"
#include "ServerSocket.hpp"
#include "PollServer.hpp"

int main(int ac, char **av){
	(void)av;
	if (ac != 2){
		std::cerr << "Usage: <./webserv config.conf>" << std::endl;
		return (1);
	}
	try {
		ServerConfig config;
		config.listen = 8080;

		PollServer pollServer;
		pollServer.addServer(config);
		std::cout << "Server listening on port 8080" << std::endl;
		pollServer.runServer();
	}
	catch (std::exception &e){
		std::cerr << e.what() << std::endl;
		return (1);
	}
	return (0);
}
