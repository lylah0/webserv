#include <iostream>
#include "PollServer.hpp"
#include "Parser.hpp"
#include <iostream>

int main(int argc, char **argv) {
	try {
		ConfigParser parser(argc, argv);
		parser.parse();

		const std::vector<ServerConfig> &servers = parser.getServers();

		for (size_t i = 0; i < servers.size(); ++i) {
			const ServerConfig &s = servers[i];
			std::cout << "---- SERVER " << i << " ----\n";
			std::cout << "server_name: " << s.server_name << "\n";
			std::cout << "listen: " << s.listen << "\n";
			std::cout << "host: " << s.host << "\n";
			std::cout << "root: " << s.root << "\n";
			std::cout << "index: " << s.index << "\n";
			std::cout << "client_max_body_size: " << s.client_max_body_size << "\n";
			std::cout << "error_page: " << s.error_page << "\n";
	}
	}
	catch (std::exception &e) {
		std::cerr << "Error: " << e.what() << "\n";
	}
}
