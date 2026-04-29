#ifndef PARSER_HPP
#define PARSER_HPP

#include "PollServer.hpp"
#include "ServerSocket.hpp"
#include "SocketUtils.hpp"
#include "ClientConnection.hpp"
#include <stdexcept>
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>

struct LocationConfig {
    std::string              path;
    std::vector<std::string> methods;
    std::string              root;
    std::string              index;
    bool                     autoindex;
    std::string              upload_store;
    bool                     upload_enabled;
    std::string              redirect;
    std::map<std::string, std::string> cgi;
};

struct ServerConfig {
    std::string server_name;
    int listen;
    std::string host;
    std::string root;
    std::string index;
    size_t client_max_body_size;
    std::string error_page;
    std::vector<LocationConfig> locations;
};

class ConfigParser
{
    private :
        std::string filename;
        std::vector<ServerConfig> servers;
    public :
        ConfigParser(int argc, char** argv);
        void parse();
        const std::vector<ServerConfig>& getServers() const;
        void parseTokens(const std::vector<std::string> &tokens);
};

#endif