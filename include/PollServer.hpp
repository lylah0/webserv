/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   PollServer.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lylrandr <lylrandr@student.42lausanne.ch>  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/14 16:50:23 by lylrandr          #+#    #+#             */
/*   Updated: 2026/04/27 13:53:12 by lylrandr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef POLLSERVER_HPP
#define POLLSERVER_HPP

# include "ClientState.hpp"
# include "ClientConnection.hpp"
# include "ServerSocket.hpp"
# include <poll.h>
# include <vector>
# include <map>

class PollServer{
	private :
		std::vector<pollfd>					_fds;
		std::vector<ServerSocket*>			_servers;
		std::map<int, ClientConnection*>	_clients;
		std::map<int, ClientState>			_states;

		PollServer(const PollServer &src);
		PollServer&							operator=(const PollServer &rhs);
		void								_addFd(int fd);
		void								_removeFd(int fd);
		void								_newConnection(int serverFd);
		void								_clientEvent(size_t index);
		void								_enableWrite(int fd);

	public :
		PollServer();
		~PollServer();

		void	addServer(ServerConfig const &server);
		void	runServer();
};

#endif
