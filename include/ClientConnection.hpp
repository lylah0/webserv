#ifndef CLIENTCONNECTION_HPP
#define CLIENTCONNECTION_HPP

#include "ClientConnection.hpp"
#include <stdexcept>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <iostream>

class ClientConnection
{
    private :
        int _fd;
    public :
        ClientConnection(int fd);
        ~ClientConnection();

        void handle();
};

#endif