/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: cjauregu <cjauregu@student.42lausanne.c    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/09 14:54:32 by lylrandr          #+#    #+#             */
/*   Updated: 2026/05/12 22:38:03 by cjauregu         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "SocketUtils.hpp"
#include "ServerSocket.hpp"
#include "PollServer.hpp"
#include "Parser.hpp"
#include <signal.h>

int main(int ac, char **av){
	(void)av;

	signal(SIGPIPE, SIG_IGN);
	if (ac != 2){
		std::cerr << "Usage: <./webserv config.conf>" << std::endl;
		return (1);
	}
	try {
		ConfigParser parser(ac, av);
		parser.parse();
		std::vector<ServerConfig> const &servers = parser.getServers();

		PollServer pollServer;
		for (size_t i = 0; i < servers.size(); i++)
			pollServer.addServer(servers[i]);
		for (size_t i = 0; i < servers.size(); i++)
			std::cout << "Server " << servers[i].server_name << " listening on : " << servers[i].listen << std::endl;
		pollServer.runServer();
	}
	catch (std::exception &e){
		std::cerr << e.what() << std::endl;
		return (1);
	}
	return (0);
}
