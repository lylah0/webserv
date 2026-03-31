#include "PollServer.hpp"
#include <iostream>

int main() {
    try {
        std::vector<int> ports;
        ports.push_back(8080);
        PollServer server(ports);
        std::cout << "Listening on port 8080" << std::endl;
        server.run();
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal server error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
