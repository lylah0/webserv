#ifndef PARSER_HPP
#define PARSER_HPP

#include "ServerConfig.hpp"
#include "HttpResponse.hpp"
#include "HttpHandler.hpp"
#include "HttpRequest.hpp"
#include <stdexcept>
#include <cstdlib>
#include <fcntl.h>
#include <iomanip>
#include <unistd.h>
#include <cctype>

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

HttpResponse handleRawPostBody(const HttpRequest& request, const LocationConfig& location, const ServerConfig& server);
HttpResponse parseMultipartAndSave(const std::string& body, const std::string& boundary, const LocationConfig& location, const ServerConfig& server);
HttpResponse debugDumpMultipart(const HttpRequest& request, const std::string& boundary);
HttpResponse makeUploadResponse(int status, const std::string& message, const std::string& extra);

#endif
