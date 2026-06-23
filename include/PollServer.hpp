/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   PollServer.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lylrandr <lylrandr@student.42lausanne.ch>  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/14 16:50:23 by lylrandr          #+#    #+#             */
/*   Updated: 2026/06/23 23:09:53 by lylrandr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef POLLSERVER_HPP
#define POLLSERVER_HPP

# include "ClientState.hpp"
# include "ClientConnection.hpp"
# include "ServerSocket.hpp"
# include "HttpRequest.hpp"
# include "HttpResponse.hpp"
# include "HttpHandler.hpp"
# include "CGI.hpp"
# include <poll.h>
# include <vector>
# include <map>
# include <ctime>
#include "CGIProcess.hpp"
# include <signal.h>

class ServerSocket;

class PollServer {
    private:
        std::vector<pollfd>                 _fds;
        std::vector<ServerSocket*>           _servers;
        std::map<int, ClientConnection*>    _clients;
        std::map<int, ClientState>          _states;
        std::vector<ServerConfig>           _configs;
        std::map<int, ServerConfig>         _clientConfig;
        std::map<int, CGIProcess>           _cgiProcesses;
        std::map<int, int>                  _pipeToClient;

        PollServer(const PollServer &src);
        PollServer& operator=(const PollServer &rhs);
		void	_handleDisconnect(int clientFd);
		void	_dispatchRequest(int clientFd, ClientConnection *client, ClientState &state);
        void    _addFd(int fd, short events = POLLIN);
        void    _removeFd(int fd);
        void    _newConnection(int serverFd);
        void    _clientEvent(size_t index);
        void    _enableWrite(int fd);
        void    _disableWrite(int fd);

        void    _handleCGIWrite(int pipeFd);
        void    _handleCGIRead(int pipeFd);
        void    _finishCGI(int clientFd);
        void    _abortCGI(int clientFd);

		bool    decodeChunkedBody(ClientConnection* client, ClientState& state);
		bool	_assembleBody(int clientFd, ClientConnection *client, ClientState &state);
		bool	_parseHeaders(int clientFd, ClientConnection *client, ClientState &state);

    public:
        PollServer();
        ~PollServer();

        void    addServer(ServerConfig const &server);
        void    runServer();
        void    registerCGI(int clientFd, CGIProcess &cgi);

		bool	_buildFinalRequest(ClientConnection *client, ClientState &state, HttpRequest &request);
		bool	_handleRedirect(int clientFd, ClientConnection *client, ClientState &state, const LocationConfig &loc);
		bool	_handleCGI(int clientFd, ClientConnection *client, ClientState &state, const HttpRequest &request, const LocationConfig &loc);
	};

#endif
