#ifndef CLIENTSTATE_HPP
#define CLIENTSTATE_HPP

#include <vector>
#include <map>
#include <poll.h>
#include <unistd.h>
#include <iostream>
#include <stdexcept>

class ClientState {
    public:
        std::string buffer;

        bool headersComplete;
        bool requestReady;

        size_t headerEnd;
        size_t contentLength;

        std::string method;
        std::string target;
        std::string version;
        std::map<std::string, std::string> headers;

        std::string body;

        bool keepAlive;

        ClientState();
};

#endif