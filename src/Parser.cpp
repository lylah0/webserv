#include "Parser.hpp"

ConfigParser::ConfigParser(int argc, char** argv)
{
    if (argc != 2)
        throw std::runtime_error("Incorrect arguments\n");
    filename = argv[1];
}

const std::vector<ServerConfig>& ConfigParser::getServers() const
{
    return servers;
}

std::vector<std::string> tokenize(const std::string& text)
{
    std::vector<std::string> tokens;
    std::string current;

    for (size_t i = 0; i < text.size(); ++i)
    {
        char c = text[i];
        if (isspace(c))
        {
            if (!current.empty())
            {
                tokens.push_back(current);
                current.clear();
            }
            continue;
        }
        if (c == '{' || c == '}' || c == ';')
        {
            if (!current.empty())
            {
                tokens.push_back(current);
                current.clear();
            }
            tokens.push_back(std::string(1, c));
            continue;
        }
        current += c;
    }
    if (!current.empty())
        tokens.push_back(current);
    return tokens;
}

void ConfigParser::parseTokens(const std::vector<std::string> &tokens) {
    size_t i = 0;

    while (i < tokens.size()) {
        if (tokens[i] != "server")
            throw std::runtime_error("Expected 'server'");
        ++i;
        if (i >= tokens.size() || tokens[i] != "{")
            throw std::runtime_error("Expected '{' after server");
        ++i;

        ServerConfig cfg;
        cfg.listen = 0;
        cfg.client_max_body_size = 0;

        while (i < tokens.size() && tokens[i] != "}") {
            std::string key = tokens[i++];
            if (i >= tokens.size())
                throw std::runtime_error("Unexpected end of file");
            std::string value = tokens[i++];
            if (i >= tokens.size() || tokens[i] != ";")
                throw std::runtime_error("Expected ';'");
            ++i;
            if (key == "server_name") cfg.server_name = value;
            else if (key == "listen") cfg.listen = std::atoi(value.c_str());
            else if (key == "host") cfg.host = value;
            else if (key == "root") cfg.root = value;
            else if (key == "index") cfg.index = value;
            else if (key == "client_max_body_size") cfg.client_max_body_size = std::strtoul(value.c_str(), 0, 10);
            else if (key == "error_page") cfg.error_page = value;
            else
                throw std::runtime_error("Unknown directive: " + key);
        }
        if (i >= tokens.size() || tokens[i] != "}")
            throw std::runtime_error("Expected '}' at end of server block");
        ++i;
        servers.push_back(cfg);
    }
}

void ConfigParser::parse()
{
    int fd = open(filename.c_str(), O_RDONLY);
    if (fd < 0)
        throw std::runtime_error("Cannot open config file");

    std::string text;
    char buf[4096];
    ssize_t n;
    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        text.append(buf, n);
    }
    close(fd);
    if (n < 0)
        throw std::runtime_error("Error reading config file");
    std::vector<std::string> tokens = tokenize(text);
    parseTokens(tokens);
}
