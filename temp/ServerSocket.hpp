#ifndef SERVERSOCKET_HPP
#define SERVERSOCKET_HPP

#include <arpa/inet.h>
#include "ClientConnection.hpp"
#include <stdexcept>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <iostream>
#include <errno.h>

class ServerSocket
{
    private :
        int _fd;
        struct sockaddr_in _addr;
    public :
        ServerSocket(int port);
        int getFd() const;
        int acceptClient() const;
};

#endif