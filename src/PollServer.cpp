/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   PollServer.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lylrandr <lylrandr@student.42lausanne.ch>  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/14 17:49:36 by lylrandr          #+#    #+#             */
/*   Updated: 2026/06/25 14:23:43 by lylrandr         ###   ########.fr       */
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
    _enableWrite(clientFd);
}

/*	Orchestre le traitement d'un client prêt en lecture/écriture.
	Lit le socket, ferme proprement si déconnexion, puis enchaine
	parsing headers -> assemblage body -> dispatch. Sort tot (return a poll)
	des qu'une etape signale qu'il manque des donnees ou qu'une reponse est deja en file. */
void PollServer::_clientEvent(size_t index)
{
    int clientFd = _fds[index].fd;
    if (_clients.find(clientFd) == _clients.end())
        return;
    ClientConnection *client = _clients[clientFd];
    ClientState &state = _states[clientFd];
    if (!client->handleRead()){
        _handleDisconnect(clientFd);
        return;
    }
    if (client->getReadBuffer().empty())
        return;
    if (!_parseHeaders(clientFd, client, state))
        return;
    if (!_assembleBody(clientFd, client, state))
        return;
    _dispatchRequest(clientFd, client, state);
}

/*Nettoie et ferme un client deconnecte : abort du CGI eventuel,
 suppression de ses entrees dans les maps (_clients, _states, _clientConfig),
 retrait du fd de poll et close().*/
void PollServer::_handleDisconnect(int clientFd)
{
    _abortCGI(clientFd);
    delete _clients[clientFd];
    _clients.erase(clientFd);
    _states.erase(clientFd);
    _clientConfig.erase(clientFd);
    _removeFd(clientFd);
    close(clientFd);
}
/*	Parse la ligne de requete + les headers une fois "\r\n\r\n" recu.
	Determine le mode du body (chunked via Transfer-Encoding, sinon Content-Length),
	fixe maxBodySize depuis la route, et rejette tot (413) si Content-Length depasse.
	Retourne false si headers incomplets (on attend plus de donnees) ou si une erreur a ete envoyee.*/
bool PollServer::_parseHeaders(int clientFd, ClientConnection *client, ClientState &state){
    if (state.headersComplete)
        return true;
    const std::string &buf = client->getReadBuffer();
    size_t headerEnd = buf.find("\r\n\r\n");
    if (headerEnd == std::string::npos)
        return false;
    state.headersComplete = true;

    std::string headerBlock = buf.substr(0, headerEnd);
    if (headerBlock.find("Transfer-Encoding: chunked") != std::string::npos){
        state.isChunked = true;
        state.contentLength = 0;
    }
    else {
        size_t clPos = headerBlock.find("Content-Length:");
        if (clPos == std::string::npos)
            state.contentLength = 0;
        else {
            clPos += 15;
            while (clPos < headerBlock.size() && headerBlock[clPos] == ' ')
                ++clPos;
            state.contentLength = std::atoi(headerBlock.c_str() + clPos);
        }
    }
    HttpRequest tempReq;
    size_t consumed = 0;
    if (!parseRequestFromBuffer(buf, tempReq, consumed))
        return false;
    LocationConfig locEarly = route(tempReq, _clientConfig[clientFd]);
    state.maxBodySize = locEarly.client_max_body_size;
    if (!state.isChunked && state.maxBodySize > 0 && state.contentLength > state.maxBodySize){
        client->prepResponse(buildError(413, "Request Entity Too Large", _clientConfig[clientFd]));
        _enableWrite(clientFd);
        state = ClientState();
        state.closeAfterWrite = true;
        return false;
    }
    return true;
}

/*	Assemble le corps de la requete selon le mode detecte.
	Branche chunked : delegue a decodeChunkedBody et controle la taille.
	Branche normale : attend que bodySize atteigne contentLength puis copie le body.
	Verifie maxBodySize (413). Retourne false tant que le body n'est pas complet.*/
bool PollServer::_assembleBody(int clientFd, ClientConnection *client, ClientState &state)
{
    if (state.requestReady)
        return true;
    const std::string &buf = client->getReadBuffer();
    if (state.isChunked){
        if (!decodeChunkedBody(client, state))
            return false;
        if (state.chunkedError){
            client->prepResponse(buildError(413, "Request Entity Too Large", _clientConfig[clientFd]));
            _enableWrite(clientFd);
            state = ClientState();
            state.closeAfterWrite = true;
            return false;
        }
        if (state.maxBodySize > 0 && state.bodyBytesRead > state.maxBodySize){
            client->prepResponse(buildError(413, "Request Entity Too Large", _clientConfig[clientFd]));
            _enableWrite(clientFd);
            state = ClientState();
            state.closeAfterWrite = true;
            return false;
        }
        if (!state.requestReady)
            return false;
    }
    else {
        size_t headerEnd = buf.find("\r\n\r\n");
        size_t bodyStart = headerEnd + 4;
        size_t bodySize = buf.size() - bodyStart;
        if (bodySize < state.contentLength)
            return false;
        if (state.maxBodySize > 0 && bodySize > state.maxBodySize){
            client->prepResponse(buildError(413, "Request Entity Too Large", _clientConfig[clientFd]));
            _enableWrite(clientFd);
            state = ClientState();
            state.closeAfterWrite = true;
            return false;
        }
        state.body = buf.substr(bodyStart, bodySize);
        state.bodyBytesRead = bodySize;
        state.requestReady = true;
    }
    return true;
}


/*	Aiguille une requete complete : construit le HttpRequest final,
	resout la route, puis tente redirection -> CGI -> sinon execute() classique.
	Prepare la reponse et active l'ecriture (POLLOUT).*/
void PollServer::_dispatchRequest(int clientFd, ClientConnection *client, ClientState &state)
{
    HttpRequest request;
    if (!_buildFinalRequest(client, state, request))
        return;
    LocationConfig loc = route(request, _clientConfig[clientFd]);
    if (_handleRedirect(clientFd, client, state, loc))
        return;
    if (_handleCGI(clientFd, client, state, request, loc))
        return;
    HttpResponse response = execute(request, loc, _clientConfig[clientFd]);
    client->prepResponse(response);
    _enableWrite(clientFd);
    state = ClientState();
}

/*	Reconstruit le HttpRequest final a partir du buffer.
	Cas chunked : re-parse les headers seuls et rattache le body deja decode.
	Cas normal : parseRequestFromBuffer. Consomme les octets traites (popReadBytes).
	Retourne false si le parsing echoue.*/
bool PollServer::_buildFinalRequest(ClientConnection *client, ClientState &state, HttpRequest &request)
{
    const std::string &buf = client->getReadBuffer();
    size_t consumed = 0;
    if (state.isChunked){
        size_t headerEnd = buf.find("\r\n\r\n");
        if (headerEnd == std::string::npos)
            return false;
        request = parseRequest(buf.substr(0, headerEnd + 4), headerEnd + 4, 0);
        request.body.swap(state.body);
        consumed = state.pos;
    }
    else {
        if (!parseRequestFromBuffer(buf, request, consumed))
            return false;
    }
    client->popReadBytes(consumed);
    return true;
}

/*	Traite une redirection si la route en definit une (301 Moved Permanently).
	Prepare la reponse, active l'ecriture, reset l'etat. Retourne true si une
	redirection a ete emise, false sinon (la requete continue son chemin normal).*/
bool PollServer::_handleRedirect(int clientFd, ClientConnection *client, ClientState &state, const LocationConfig &loc)
{
    if (loc.redirect.empty())
        return false;
    HttpResponse redir;
    redir.statusCode = 301;
    redir.statusMessage = "Moved Permanently";
    redir.headers["Location"] = loc.redirect;
    redir.headers["Content-Length"] = "0";
    client->prepResponse(redir);
    _enableWrite(clientFd);
    state = ClientState();
    return true;
}

/*	Tente de traiter la requete comme un CGI.
	Resout le chemin, choisit le bon location block par extension, valide le script.
	Lance le CGI ou emet l'erreur adequate (400/403/404/500). Retourne true si la
	requete a ete prise en charge ici, false si ce n'est pas un CGI (-> execute()).*/
bool PollServer::_handleCGI(int clientFd, ClientConnection *client, ClientState &state, const HttpRequest &request, const LocationConfig &loc)
{
    std::string test_path = resolvePath(request, loc);
    std::cerr << "RESOLVEPATH : " << test_path << std::endl;
    std::string ext = getExtension(test_path);
    const LocationConfig* extLocPtr = findExtensionLocation(_clientConfig[clientFd], request.uri, test_path, ext);
    const LocationConfig& extLoc = (extLocPtr ? *extLocPtr : loc);
	// std::string path = resolvePath(request, extLoc);
    std::string path = extLoc.root + request.uri;
    std::cerr << "PATH RESOLVED AS : " << path << std::endl;
    std::cerr << "CGI LOCATION BLOCK CHOSEN : " << extLoc.path << std::endl;
    std::cerr << "CGI LOCATION BLOCK DEFAULT : " << loc.path << std::endl;
    if (!isCGIvalid(extLoc, ext, request.method) || (request.method != "GET" && request.method != "POST"))
        return false;
    std::cerr << "[CGI] validation check" << std::endl;
    std::string isvalid = CGI_validation_check(path);
    if (isvalid != "CGI validated" && isvalid != "Non CGI"){
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
        return true;
    }
    if (isvalid == "CGI validated"){
        try {
            CGIProcess cgi = launchCGI(request, _clientConfig[clientFd], extLoc, path);
            registerCGI(clientFd, cgi);
        } catch (...) {
            client->prepResponse(buildError(500, "Internal Server Error", _clientConfig[clientFd]));
            _enableWrite(clientFd);
            state = ClientState();
            state.closeAfterWrite = true;
            return true;
        }
        state = ClientState();
        return true;
    }
    return false;
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
