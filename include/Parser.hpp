#ifndef PARSER_HPP
#define PARSER_HPP

#include "ServerConfig.hpp"
#include <stdexcept>
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>

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