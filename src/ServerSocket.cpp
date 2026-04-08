#include "ServerSocket.hpp"
#include "SocketUtils.hpp"

ServerSocket::ServerSocket(int port)
{
    _fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_fd < 0)
        throw std::runtime_error("socket failed");
    int opt = 1;
    if (setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        close(_fd);
        throw std::runtime_error("setsockopt failed");
    }
    std::memset(&_addr, 0, sizeof(_addr));
    _addr.sin_family = AF_INET;
    _addr.sin_addr.s_addr = INADDR_ANY;
    _addr.sin_port = htons(port);
    if (bind(_fd, (struct sockaddr*)&_addr, sizeof(_addr)) < 0)
    {
        close(_fd);
        throw std::runtime_error("bind() failed");
    }
    if (listen(_fd, 128) < 0)
    {
        close(_fd);
        throw std::runtime_error("listen() failed");
    }
    if (set_nonblocking(_fd) < 0)
    {
        close(_fd);
        throw std::runtime_error("set_nonblocking failed");
    }
}

int ServerSocket::getFd() const
{
    return _fd;
}

int ServerSocket::acceptClient() const
{
    int client = accept(_fd, NULL, NULL);
    if (client < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return -1;
        throw std::runtime_error("accept() failed");
    }
    return client;
}
