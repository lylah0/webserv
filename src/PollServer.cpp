/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   PollServer.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lylrandr <lylrandr@student.42lausanne.ch>  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/14 17:49:36 by lylrandr          #+#    #+#             */
/*   Updated: 2026/06/23 21:40:54 by lylrandr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "PollServer.hpp"

PollServer::PollServer() {}

PollServer::~PollServer() {
    for (std::map<int, ClientConnection*>::iterator it = _clients.begin();
         it != _clients.end(); it++)
        delete it->second;
    for (size_t i = 0; i < _servers.size(); i++)
        delete _servers[i];
    for (std::map<int, CGIProcess>::iterator it = _cgiProcesses.begin();
         it != _cgiProcesses.end(); ++it) {
        CGIProcess &cgi = it->second;
        if (cgi.pid != -1)
            kill(cgi.pid, SIGKILL);
        if (cgi.inFd != -1)  close(cgi.inFd);
        if (cgi.outFd != -1) close(cgi.outFd);
    }
}

void PollServer::_addFd(int fd, short events) {
    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = events;
    pfd.revents = 0;
    _fds.push_back(pfd);
}

void PollServer::_removeFd(int fd) {
    for (size_t i = 0; i < _fds.size(); i++) {
        if (_fds[i].fd == fd) {
            _fds.erase(_fds.begin() + i);
            return;
        }
    }
}

void PollServer::_enableWrite(int fd) {
    for (size_t i = 0; i < _fds.size(); i++) {
        if (_fds[i].fd == fd) {
            _fds[i].events |= POLLOUT;
            return;
        }
    }
}

void PollServer::_disableWrite(int fd) {
    for (size_t i = 0; i < _fds.size(); i++) {
        if (_fds[i].fd == fd) {
            _fds[i].events &= ~POLLOUT;
            return;
        }
    }
}

void PollServer::_newConnection(int serverFd) {
    for (size_t i = 0; i < _servers.size(); i++) {
        if (_servers[i]->getFd() == serverFd) {
            int clientFd = _servers[i]->acceptClient();
            if (clientFd < 0)
                return;
            setNonBlocking(clientFd);
            _addFd(clientFd);
            _clients[clientFd] = new ClientConnection(clientFd);
            _states[clientFd] = ClientState();
            _clientConfig[clientFd] = _configs[i];
            return;
        }
    }
}

void PollServer::registerCGI(int clientFd, CGIProcess &cgi) {
    _cgiProcesses[clientFd] = cgi;
    _pipeToClient[cgi.outFd] = clientFd;
    _addFd(cgi.outFd, POLLIN);

    //std::cerr << "CGI REGISTERED" << std::endl;
    if (!cgi.inputDone) {
        _pipeToClient[cgi.inFd] = clientFd;
        _addFd(cgi.inFd, POLLOUT);
    } else {
        close(cgi.inFd);
        _cgiProcesses[clientFd].inFd = -1;
    }
}

void PollServer::_handleCGIWrite(int pipeFd)
{
    std::map<int,int>::iterator itClient = _pipeToClient.find(pipeFd);
    //std::cerr << "CGI HANDLEWRITE CALLED" << std::endl;
    if (itClient == _pipeToClient.end())
        return;

    int clientFd = itClient->second;
    CGIProcess &cgi = _cgiProcesses[clientFd];

    if (cgi.inputDone) {
        _removeFd(pipeFd);
        return;
    }

    const std::string &buf = cgi.inputBuffer;
    size_t remaining = buf.size() - cgi.inputOffset;
    if (remaining == 0) {
        close(cgi.inFd);
        cgi.inputDone = true;
        _removeFd(pipeFd);
        return;
    }
    ssize_t n = write(pipeFd, buf.data() + cgi.inputOffset, remaining);
    //std::cerr << "[CGI WRITE] write returned " << n << "\n";
    if (n > 0) {
        cgi.inputOffset += n;
        if (cgi.inputOffset >= buf.size()) {
            close(cgi.inFd);
            cgi.inputDone = true;
            _removeFd(pipeFd);
        }
        return;
    }
    close(cgi.inFd);
    cgi.inputDone = true;
    _removeFd(pipeFd);
}

bool PollServer::decodeChunkedBody(ClientConnection* client, ClientState& state)
{
    const std::string& buf = client->getReadBuffer();
    if (state.pos == 0) {
        size_t headerEnd = buf.find("\r\n\r\n");
        if (headerEnd == std::string::npos) {
            //DBG("waiting for header end");
            return false;
        }
        state.pos = headerEnd + 4;
        //DBG2("init pos", state.pos);
    }

    while (true)
    {
        if (state.haveChunkSize && state.currentChunkSize == 0)
        {
            while (true)
            {
                size_t lineEnd = buf.find("\r\n", state.pos);
                if (lineEnd == std::string::npos) {
                    //DBG("waiting for final CRLF or trailer line");
                    return false;
                }
                if (lineEnd == state.pos) {
                    state.pos += 2;
                    state.requestReady = true;
                    //DBG("final chunk reached, body complete (trailers done)");
                    return true;
                }
                std::string trailer = buf.substr(state.pos, lineEnd - state.pos);
                state.pos = lineEnd + 2;
            }
        }
        if (!state.haveChunkSize)
        {
            size_t lineEnd = buf.find("\r\n", state.pos);
            if (lineEnd == std::string::npos) {
                return false;
            }

            std::string hex = buf.substr(state.pos, lineEnd - state.pos);
            if (hex.empty()) {
                //DBG("empty line before chunk-size, skipping");
                state.pos = lineEnd + 2;
                continue;
            }
            state.currentChunkSize = strtol(hex.c_str(), NULL, 16);
            state.haveChunkSize = true;
            //DBG2("chunk-size", hex);
            //DBG2("chunk-size-dec", state.currentChunkSize);
            state.pos = lineEnd + 2;
            if (state.currentChunkSize == 0) {
                //DBG("entered final-chunk mode (size 0)");
                continue;
            }
        }
        if (state.currentChunkSize == 0) {
            //DBG("BUG: entered data branch with size 0 — forcing final-chunk mode");
            continue;
        }

        if (buf.size() < state.pos + state.currentChunkSize) {
            //DBG("waiting for chunk data");
            return false;
        }
        //DBG2("append-bytes", state.currentChunkSize);
        state.body.append(buf.substr(state.pos, state.currentChunkSize));
        state.bodyBytesRead += state.currentChunkSize;
        state.pos += state.currentChunkSize;
        state.haveChunkSize = false;
        //DBG("chunk data complete, treating next bytes as start of next size line");
    }
}

void PollServer::_handleCGIRead(int pipeFd)
{
    std::map<int,int>::iterator itClient = _pipeToClient.find(pipeFd);
    //std::cerr << "CGI HANDLEREAD CALLED" << std::endl;
    if (itClient == _pipeToClient.end())
        return;
    int clientFd = itClient->second;
    CGIProcess &cgi = _cgiProcesses[clientFd];
    char buf[4096];
    ssize_t n = read(pipeFd, buf, sizeof(buf));
    //std::cerr << "\n[CGI_READ] SIZE OF BUFFER READING PIPEFD OUT N : " << n << std::endl;
    if (n > 0) {
        /*size_t preview = (n > 200 ? 200 : n);
        std::cerr << "data_preview (" << preview << " bytes):\n";
        std::cerr << std::string(buf, preview) << "\n";
        std::cerr << "--- END PREVIEW ---\n";*/
        cgi.outputBuffer.append(buf, n);
        return;
    }
    _removeFd(pipeFd);
    close(pipeFd);
    cgi.outputDone = true;
    int status;
    pid_t r = waitpid(cgi.pid, &status, WNOHANG);
    std::cerr << "[CGI PARENT] waitpid=" << r << "\n";

    std::string outputBuffer = cgi.outputBuffer;
    _pipeToClient.erase(pipeFd);
    if (cgi.inFd != -1)
        _pipeToClient.erase(cgi.inFd);
    _cgiProcesses.erase(clientFd);
    if (_clients.find(clientFd) == _clients.end())
        return;
    HttpResponse res = parseCGIOutput(outputBuffer);
    if (res.statusCode == 500)
    {
        //std::cerr << "[CGI] ERROR STATUS MESSAGE FOUND HERE : " << res.statusMessage << std::endl;
        res = buildError(500, "Internal Server Error", _clientConfig[clientFd]);
    }
    //std::cerr << "[CGI READ] n=" << n << "\n";
    ClientConnection *client = _clients[clientFd];
    client->prepResponse(res);
    client->handleWrite();
    _enableWrite(clientFd);
}

void PollServer::_finishCGI(int clientFd) {
    std::map<int, CGIProcess>::iterator it = _cgiProcesses.find(clientFd);
    //std::cerr << "CGI FINISHED" << std::endl;
    if (it == _cgiProcesses.end())
        return;
    pid_t pid  = it->second.pid;
    int   inFd = it->second.inFd;
    std::string output = it->second.outputBuffer;
    int status;
    waitpid(pid, &status, WNOHANG);
    if (inFd != -1) {
        _removeFd(inFd);
        _pipeToClient.erase(inFd);
        close(inFd);
    }
    _cgiProcesses.erase(clientFd);
    if (_clients.find(clientFd) == _clients.end())
        return;
    HttpResponse response = parseCGIOutput(output);
    if (response.statusCode == 500)
        response = buildError(500, "Internal Server Error2", _clientConfig[clientFd]);
    _clients[clientFd]->prepResponse(response);
    _enableWrite(clientFd);
}

void PollServer::_abortCGI(int clientFd) {
    std::map<int, CGIProcess>::iterator it = _cgiProcesses.find(clientFd);
    //std::cerr << "CGI ABORTED" << std::endl;
    if (it == _cgiProcesses.end())
        return;
    pid_t pid   = it->second.pid;
    int   inFd  = it->second.inFd;
    int   outFd = it->second.outFd;
    if (pid != -1) {
        kill(pid, SIGKILL);
        waitpid(pid, NULL, WNOHANG);
    }
    if (inFd != -1) {
        _removeFd(inFd);
        _pipeToClient.erase(inFd);
        close(inFd);
    }
    if (outFd != -1) {
        _removeFd(outFd);
        _pipeToClient.erase(outFd);
        close(outFd);
    }
    _cgiProcesses.erase(clientFd);
    if (_clients.find(clientFd) == _clients.end())
        return;
    ClientConnection *client = _clients[clientFd];
    client->prepResponse(buildError(504, "Gateway Timeout", _clientConfig[clientFd]));
    client->handleWrite();
    _enableWrite(clientFd);
}

void PollServer::_clientEvent(size_t index)
{
    int clientFd = _fds[index].fd;
    if (_clients.find(clientFd) == _clients.end())
        return;

    ClientConnection *client = _clients[clientFd];
    ClientState &state = _states[clientFd];
    if (!client->handleRead()) {
        _abortCGI(clientFd);
        //std::cerr << "CGI ABORTED RIGHT HERE AFTER CHECKING HANDLEREAD" << std::endl;
        delete client;
        _clients.erase(clientFd);
        _states.erase(clientFd);
        _clientConfig.erase(clientFd);
        _removeFd(clientFd);
        close(clientFd);
        return;
    }
    while (true)
    {
        const std::string &buf = client->getReadBuffer();
        if (buf.empty())
            return;
        if (!state.headersComplete)
        {
            size_t headerEnd = buf.find("\r\n\r\n");
            if (headerEnd == std::string::npos)
                break;
            state.headersComplete = true;
            HttpRequest tempReq;
            size_t consumed = 0;
            bool ok = parseRequestFromBuffer(buf, tempReq, consumed);
            if (!ok) {
                size_t headerEnd = buf.find("\r\n\r\n");
                if (headerEnd != std::string::npos) {
                    size_t bodyStart = headerEnd + 4;
                    size_t bodySize = buf.size() - bodyStart;
                    if (bodySize < state.contentLength) {
                        break;
                    }
                }
                client->prepResponse(buildError(400, "Bad Requestballs", _clientConfig[clientFd]));
                _enableWrite(clientFd);
                state = ClientState();
                state.closeAfterWrite = true;
                return;
            }
            LocationConfig locEarly = route(tempReq, _clientConfig[clientFd]);
            state.maxBodySize = locEarly.client_max_body_size;
            if (tempReq.headers.count("Transfer-Encoding") &&
                tempReq.headers.at("Transfer-Encoding") == "chunked")
            {
                state.isChunked = true;
                state.contentLength = 0;
            }
            else if (tempReq.headers.count("Content-Length"))
            {
                state.contentLength = std::atoi(tempReq.headers.at("Content-Length").c_str());
            }
            else
            {
                state.contentLength = 0;
            }
            if (!state.isChunked &&
                state.maxBodySize > 0 &&
                state.contentLength > state.maxBodySize)
            {
                client->prepResponse(buildError(413, "Request Entity Too Large", _clientConfig[clientFd]));
                _enableWrite(clientFd);
                state = ClientState();
                state.closeAfterWrite = true;
                return;
            }
        }
        if (!state.requestReady)
        {
            if (state.isChunked)
            {
                if (!decodeChunkedBody(client, state))
                    break;
                if (state.chunkedError)
                {
                    client->prepResponse(buildError(413, "Request Entity Too Large", _clientConfig[clientFd]));
                    _enableWrite(clientFd);
                    state = ClientState();
                    state.closeAfterWrite = true;
                    return;
                }

                if (state.maxBodySize > 0 &&
                    state.bodyBytesRead > state.maxBodySize)
                {
                    client->prepResponse(buildError(413, "Request Entity Too Large", _clientConfig[clientFd]));
                    _enableWrite(clientFd);
                    state = ClientState();
                    state.closeAfterWrite = true;
                    return;
                }

                if (!state.requestReady)
                    return;
            }
            else
            {
                size_t headerEnd = buf.find("\r\n\r\n");
                size_t bodyStart = headerEnd + 4;
                size_t bodySize = buf.size() - bodyStart;

                if (bodySize < state.contentLength)
                    return;

                if (state.maxBodySize > 0 &&
                    bodySize > state.maxBodySize)
                {
                    client->prepResponse(buildError(413, "Request Entity Too Large", _clientConfig[clientFd]));
                    _enableWrite(clientFd);
                    state = ClientState();
                    state.closeAfterWrite = true;
                    return;
                }

                state.body = buf.substr(bodyStart, bodySize);
                state.bodyBytesRead = bodySize;
                state.requestReady = true;
            }
        }

        if (!state.requestReady)
            return;
        HttpRequest request;
        size_t consumed = 0;

        if (state.isChunked)
        {
            size_t headerEnd = buf.find("\r\n\r\n");
            if (headerEnd == std::string::npos)
                break;

            request = parseRequest(buf.substr(0, headerEnd + 4), headerEnd + 4, 0);
            request.body = state.body;
            consumed = state.pos;
        }
        else
        {
            if (!parseRequestFromBuffer(buf, request, consumed))
                break;
        }
        client->popReadBytes(consumed);
        LocationConfig loc = route(request, _clientConfig[clientFd]);
        if (!loc.redirect.empty())
        {
            HttpResponse redir;
            redir.statusCode = 301;
            redir.statusMessage = "Moved Permanently";
            redir.headers["Location"] = loc.redirect;
            redir.headers["Content-Length"] = "0";

            client->prepResponse(redir);
            _enableWrite(clientFd);
            state = ClientState();
            return;
        }
        std::string test_path = resolvePath(request, loc);
        std::cerr << "RESOLVEPATH : " << test_path << std::endl;
        std::string ext = getExtension(test_path);
        const LocationConfig* extLocPtr = findExtensionLocation(_clientConfig[clientFd], request.uri, test_path, ext);
        const LocationConfig& extLoc = (extLocPtr ? *extLocPtr : loc);
        std::string path = resolvePath(request, extLoc);
        std::cerr << "PATH RESOLVED AS : " << path << std::endl;
        std::cerr << "CGI LOCATION BLOCK CHOSEN : " << extLoc.path << std::endl;
        std::cerr << "CGI LOCATION BLOCK DEFAULT : " << loc.path << std::endl;
        if (isCGIvalid(extLoc, ext, request.method) && (request.method == "GET" || request.method == "POST"))
        {
            std::cerr << "[CGI] validation check" << std::endl;
            std::string isvalid = CGI_validation_check(path);
            if (isvalid != "CGI validated" && isvalid != "Non CGI")
            {
                HttpResponse response;
                if (isvalid == "Extension error" || isvalid == "Is regular file")
                    response = buildError(400, "Bad Request", _clientConfig[clientFd]);
                else if (isvalid == "File non existent")
                    response = buildError(404, "Not Found", _clientConfig[clientFd]);
                else if (isvalid == "Not executable")
                    response = buildError(403, "Forbidden", _clientConfig[clientFd]);
                else
                    response = buildError(500, "Internal Server Error", _clientConfig[clientFd]);
                client->prepResponse(response);
                _enableWrite(clientFd);
                state = ClientState();
                state.closeAfterWrite = true;
                return;
            }
            if (isvalid == "CGI validated")
            {
                //std::cerr << "[CGI] CGI VALIDATED AND PROCESSING..." << std::endl;
                //std::cerr << "[CGI] CGI LOCATION GIVEN : " << extLoc->path << std::endl;
                try {
                    CGIProcess cgi = launchCGI(request, _clientConfig[clientFd], extLoc, path);
                    registerCGI(clientFd, cgi);
                } catch (...) {
                    client->prepResponse(buildError(500, "Internal Server Error", _clientConfig[clientFd]));
                    _enableWrite(clientFd);
                    state = ClientState();
                    state.closeAfterWrite = true;
                    return;
                }
                state = ClientState();
                return;
            }
            //if (isvalid == "Non CGI")
                //std::cerr << "[CGI] Program marked file as non CGI" << std::endl;
        }
        HttpResponse response = execute(request, loc, _clientConfig[clientFd]);
        client->prepResponse(response);
        _enableWrite(clientFd);

        state = ClientState();
        return;
    }
}

void PollServer::addServer(ServerConfig const &server) {
    _configs.push_back(server);
    _servers.push_back(new ServerSocket(server));
    _addFd(_servers.back()->getFd());
}

void PollServer::runServer() {
    while (1) {
        time_t now = time(NULL);
        for (std::map<int, CGIProcess>::iterator it = _cgiProcesses.begin();
             it != _cgiProcesses.end(); ) {
            if (now - it->second.startTime > 30) {
                int clientFd = it->first;
                std::cerr << "[CGI] Timeout for client fd=" << clientFd << "\n";
                ++it;
                _abortCGI(clientFd);
            } else {
                ++it;
            }
        }
        int ret = poll(&_fds[0], _fds.size(), 1000);
        if (ret < 0) {
            if (errno == EINTR)
                continue;
            throw std::runtime_error("poll() failed");
        }
        for (size_t i = 0; i < _fds.size(); ++i) {
            int   fd      = _fds[i].fd;
            short revents = _fds[i].revents;

            if (revents == 0)
                continue;
            _fds[i].revents = 0;
            size_t sizeBefore = _fds.size();

            if (_pipeToClient.find(fd) != _pipeToClient.end()) {
                int clientFd = _pipeToClient[fd];
                if (_cgiProcesses.find(clientFd) == _cgiProcesses.end())
                    continue;
                CGIProcess &cgi = _cgiProcesses[clientFd];

                if (fd == cgi.inFd && (revents & POLLOUT)) {
                    _handleCGIWrite(fd);
                } else if (fd == cgi.outFd && (revents & POLLIN)) {
                    _handleCGIRead(fd);
                } else if (revents & (POLLERR | POLLHUP)) {
                    if (fd == cgi.outFd)
                        _handleCGIRead(fd);
                }

                if (_fds.size() < sizeBefore)
                    i -= (sizeBefore - _fds.size());
                continue;
            }

            bool isServer = false;
            for (size_t j = 0; j < _servers.size(); ++j) {
                if (_servers[j]->getFd() == fd) {
                    isServer = true;
                    break;
                }
            }
            if (isServer) {
                if (revents & POLLIN)
                    _newConnection(fd);
                continue;
            }

            if (_clients.find(fd) == _clients.end())
                continue;

            if (revents & POLLIN) {
                for (size_t k = 0; k < _fds.size(); ++k) {
                    if (_fds[k].fd == fd) {
                        _clientEvent(k);
                        break;
                    }
                }
                if (_fds.size() < sizeBefore) {
                    i -= (sizeBefore - _fds.size());
                    continue;
                }
                sizeBefore = _fds.size();
            }

            if (revents & POLLOUT) {
                if (_clients.find(fd) == _clients.end()) {
                    if (_fds.size() < sizeBefore)
                        i -= (sizeBefore - _fds.size());
                    continue;
                }
                ClientConnection *client = _clients[fd];
                if (!client->handleWrite()) {
                    _abortCGI(fd);
                    delete client;
                    _clients.erase(fd);
                    _states.erase(fd);
                    _clientConfig.erase(fd);
                    _removeFd(fd);
                    close(fd);
                    if (_fds.size() < sizeBefore)
                        i -= (sizeBefore - _fds.size());
                    continue;
                }
                if (client->writeComplete()) {
                    client->popResponse();
                    if (_states[fd].closeAfterWrite) {
                        _abortCGI(fd);
                        //std::cerr << "CGI ABORTED RIGHT HERE AFTER CHECKING WRITECOMPLETE" << std::endl;
                        delete client;
                        _clients.erase(fd);
                        _states.erase(fd);
                        _clientConfig.erase(fd);
                        _removeFd(fd);
                        close(fd);
                        if (_fds.size() < sizeBefore)
                            i -= (sizeBefore - _fds.size());
                        continue;
                    }
                    if (client->hasPendingResponses()) {
                        _enableWrite(fd);
                    } else {
                        _disableWrite(fd);
                        if (client->getReadBuffer().empty()) {
                            client->resetReadState();
                            client->clearWrite();
                        }
                    }
                }
            }

            if (revents & (POLLERR | POLLHUP)) {
                if (_clients.find(fd) != _clients.end()) {
                    _abortCGI(fd);
                    //std::cerr << "CGI ABORTED RIGHT HERE AFTER CHECKING POLLHUP" << std::endl;
                    delete _clients[fd];
                    _clients.erase(fd);
                    _states.erase(fd);
                    _clientConfig.erase(fd);
                    _removeFd(fd);
                    close(fd);
                    if (_fds.size() < sizeBefore)
                        i -= (sizeBefore - _fds.size());
                }
            }
        }
    }
}
