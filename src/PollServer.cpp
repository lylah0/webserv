// src/PollServer.cpp
#include "ServerSocket.hpp"
#include "ClientConnection.hpp"
#include "SocketUtils.hpp"
#include <vector>
#include <map>
#include <poll.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <iostream>
#include <cstring>

// Simple helper to check if fd is a listening socket
static bool is_listen_fd(int fd, const std::vector<int>& listen_fds) {
    for (size_t i = 0; i < listen_fds.size(); ++i)
        if (listen_fds[i] == fd) return true;
    return false;
}

// Simple response builder used as placeholder
static std::string buildSimpleResponse() {
    std::string body = "Hello, webserv\n";
    std::string resp;
    resp += "HTTP/1.1 200 OK\r\n";
    resp += "Content-Type: text/plain\r\n";
    resp += "Content-Length: " + std::to_string(body.size()) + "\r\n";
    resp += "Connection: close\r\n";
    resp += "\r\n";
    resp += body;
    return resp;
}

int run_poll_server(int listen_port) {
    // Create one listening socket (you can extend to multiple)
    ServerSocket server(listen_port);
    std::vector<int> listen_fds;
    listen_fds.push_back(server.getFd());

    std::vector<struct pollfd> fds;
    std::map<int, ClientConnection*> clients;

    // Add listening fds to poll set
    for (size_t i = 0; i < listen_fds.size(); ++i) {
        struct pollfd p;
        p.fd = listen_fds[i];
        p.events = POLLIN;
        p.revents = 0;
        fds.push_back(p);
    }

    while (true) {
        int ret = poll(&fds[0], fds.size(), 1000); // 1s timeout
        if (ret < 0) {
            if (errno == EINTR) continue;
            perror("poll");
            break;
        }

        // iterate over a snapshot of fds
        for (size_t i = 0; i < fds.size(); ++i) {
            struct pollfd &p = fds[i];
            if (p.revents == 0) continue;

            // Accept on listening sockets
            if ((p.revents & POLLIN) && is_listen_fd(p.fd, listen_fds)) {
                while (true) {
                    int client = accept(p.fd, NULL, NULL);
                    if (client < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                        perror("accept");
                        break;
                    }
                    if (set_nonblocking(client) < 0) {
                        close(client);
                        continue;
                    }
                    // create ClientConnection and add to poll set
                    ClientConnection *conn = new ClientConnection(client);
                    struct pollfd cp;
                    cp.fd = client;
                    cp.events = POLLIN;
                    cp.revents = 0;
                    fds.push_back(cp);
                    clients[client] = conn;
                }
                continue;
            }

            // Client events
            if (p.fd != -1 && clients.count(p.fd)) {
                ClientConnection *conn = clients[p.fd];

                // Error or hangup
                if (p.revents & (POLLERR | POLLHUP | POLLNVAL)) {
                    delete conn;
                    clients.erase(p.fd);
                    p.fd = -1;
                    continue;
                }

                // Readable
                if (p.revents & POLLIN) {
                    bool ok = conn->handleRead();
                    if (!ok) {
                        delete conn;
                        clients.erase(p.fd);
                        p.fd = -1;
                        continue;
                    }

                    // Replace this simple detection with your RequestParser integration.
                    // When a full request is available, prepare a response and enable POLLOUT.
                    if (conn->getReadBuffer().find("\r\n\r\n") != std::string::npos) {
                        std::string resp = buildSimpleResponse();
                        conn->prepareResponse(resp);
                        // enable POLLOUT for this fd
                        p.events |= POLLOUT;
                    }
                }

                // Writable
                if ((p.revents & POLLOUT) && p.fd != -1) {
                    bool ok = conn->handleWrite();
                    if (!ok) {
                        delete conn;
                        clients.erase(p.fd);
                        p.fd = -1;
                        continue;
                    }

                    // If fully sent, close connection (Connection: close)
                    if (conn->getWriteOffset() >= conn->getWriteBuffer().size()) {
                        delete conn;
                        clients.erase(p.fd);
                        p.fd = -1;
                        continue;
                    }
                }
            }
        }

        // compact fds vector removing closed fds
        std::vector<struct pollfd> newfds;
        for (size_t i = 0; i < fds.size(); ++i) {
            if (fds[i].fd != -1) newfds.push_back(fds[i]);
        }
        fds.swap(newfds);
    }

    // cleanup
    for (std::map<int, ClientConnection*>::iterator it = clients.begin(); it != clients.end(); ++it) {
        delete it->second;
    }
    clients.clear();
    return 0;
}
