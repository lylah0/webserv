#include "ClientConnection.hpp"

ClientConnection::ClientConnection(int fd) : _fd(fd) {}

ClientConnection::~ClientConnection()
{
    close(_fd);
}

void ClientConnection::handle()
{
    char buffer[4096];
    std::memset(buffer, 0, sizeof(buffer));
    int bytes = recv(_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes < 0)
        throw std::runtime_error("recv() failed");
    std::cout << "----- Received Request -----\n"
                << buffer
                << "----------------------------\n";

    const char* resp = 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 14\r\n"
        "Connection: close\r\n"
        "\r\n"
        "Hello, webserv\n";

    if (send(_fd, resp, std::strlen(resp), 0) < 0)
        throw std::runtime_error("send() failed");
}
