#ifndef POLLSERVER_HPP
#define POLLSERVER_HPP

#include <vector>
#include <map>
#include <poll.h>
#include <unistd.h>
#include "ServerSocket.hpp"
#include "ClientConnection.hpp"
#include "ClientState.hpp"
#include "HttpResponse.hpp"
#include "HttpRequest.hpp"
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <cstdlib>
#include <string>

class PollServer
{
    private :
        std::vector<ServerSocket> _listeners;
        std::vector<pollfd> _fds;
        std::map<int, ClientConnection*>    _clients;
        std::map<int, ClientState> _states;
        
        void addListener(int port);
        void addFd(int fd, short events);
        void removeFd(size_t index);
        void handleNewConnection(size_t index);
        void handleClientEvent(size_t index);
    public :
        PollServer(const std::vector<int>& ports);
        ~PollServer();

        void parseHeaders(ClientState& state);
        void parseRequest(int fd, ClientState& state);
        void processRequest(int fd, ClientState& state);
        void run();
};

#endif