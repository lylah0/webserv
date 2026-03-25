#include "ServerSocket.hpp"
#include "ClientConnection.hpp"
#include <iostream>
#include <stdexcept>

int main()
{
    try {
        ServerSocket server(8080);
        std::cout << "Listening on port : 8080" << std::endl;
        while (true) {
            try {
                int clientFd = server.acceptClient();
                ClientConnection client(clientFd);
                client.handle();
            }
            catch (const std::exception &e) {
                std::cerr << "Client error: " << e.what() << std::endl;
            }
        }
    }
    catch (const std::exception &e) {
        std::cerr << "Fatal server error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
