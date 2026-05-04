#include "PollServer.hpp"

PollServer::PollServer(const std::vector<int>& ports)
{
	for (size_t i = 0; i < ports.size(); ++i)
		addListener(ports[i]);
}

PollServer::~PollServer()
{
	for (size_t i = 0; i < _fds.size(); ++i)
		close(_fds[i].fd);
	for (std::map<int, ClientConnection*>::iterator it = _clients.begin(); it != _clients.end(); it++)
		delete it->second;
}

void PollServer::addListener(int port)
{
	_listeners.push_back(ServerSocket(port));

	pollfd p;
	p.fd = _listeners.back().getFd();
	p.events = POLLIN;
	p.revents = 0;
	_fds.push_back(p);
}

void PollServer::addFd(int fd, short events)
{
	pollfd p;
	p.fd = fd;
	p.events = events;
	p.revents = 0;
	_fds.push_back(p);
}

void PollServer::removeFd(size_t index)
{
	close(_fds[index].fd);
	_fds.erase(_fds.begin() + index);
}

void PollServer::handleNewConnection(size_t index)
{
	int serverFd = _fds[index].fd;

	for (size_t i = 0; i < _listeners.size(); ++i)
	{
		if (_listeners[i].getFd() == serverFd)
		{
			int clientFd = _listeners[i].acceptClient();
			if (clientFd < 0)
			    return;
			addFd(clientFd, POLLIN);
			_clients[clientFd] = new ClientConnection(clientFd);
			_states[clientFd] = ClientState();
		}
	}
}

void parseRequestLine(const std::string& line, ClientState& state)
{
	std::istringstream ss(line);
	ss >> state.method >> state.target >> state.version;
}

void PollServer::parseHeaders(ClientState& state)
{
	std::string headerBlock = state.buffer.substr(0, state.headerEnd);

	std::istringstream stream(headerBlock);
	std::string line;

	std::getline(stream, line);
	parseRequestLine(line, state);
	while (std::getline(stream, line) && line != "\r")
	{
		size_t colon = line.find(':');
		if (colon == std::string::npos) continue;
		std::string key = line.substr(0, colon);
		std::string value = line.substr(colon + 1);
		key.erase(key.find_last_not_of(" \t\r") + 1);
		value.erase(0, value.find_first_not_of(" \t"));
		value.erase(value.find_last_not_of(" \t\r") + 1);
		state.headers[key] = value;
	}
	if (state.headers.count("Content-Length"))
		state.contentLength = std::atoi(state.headers["Content-Length"].c_str());
	else
		state.contentLength = 0;
}

void PollServer::parseRequest(int fd, ClientState& state)
{
	if (!state.headersComplete)
	{
		size_t pos = state.buffer.find("\r\n\r\n");
		if (pos != std::string::npos)
		{
			state.headersComplete = true;
			state.headerEnd = pos + 4;
			parseHeaders(state);
		}
	}
	if (state.headersComplete && !state.requestReady)
	{
		size_t bodySize = state.buffer.size() - state.headerEnd;
		if (bodySize >= state.contentLength)
		{
			state.body = state.buffer.substr(state.headerEnd, state.contentLength);
			state.requestReady = true;
		}
	}
	if (state.requestReady)
		processRequest(fd, state);
}

void PollServer::processRequest(int fd, ClientState& state)
{
	HttpRequest req;
	req.method = state.method;
	req.target = state.target;
	req.version = state.version;
	req.headers = state.headers;
	req.body = state.body;
	HttpResponse res;
	res.statusCode = 200;
	res.statusMessage = "OK";
	res.body = "Hello from Webserv!\n";
	std::stringstream ss;
	ss << res.body.size();
	res.headers["Content-Length"] = ss.str();
	res.headers["Content-Type"] = "text/plain";
	res.headers["Connection"] = "close";
	std::stringstream response;
	response << "HTTP/1.1 " << res.statusCode << " " << res.statusMessage << "\r\n";
	std::map<std::string, std::string>::iterator it;
	for (it = res.headers.begin(); it != res.headers.end(); ++it)
		response << it->first << ": " << it->second << "\r\n";
	response << "\r\n";
	response << res.body;
	std::string out = response.str();
	send(fd, out.c_str(), out.size(), 0);
	// forbidden, need to use poll
	state = ClientState();
}


void PollServer::handleClientEvent(size_t index)
{
	int fd = _fds[index].fd;
	ClientState& state = _states[fd];

	char tmp[4096];
	ssize_t n = recv(fd, tmp, sizeof(tmp), 0);

	if (n <= 0)
	{
		delete _clients[fd];
		_clients.erase(fd);
		_states.erase(fd);
		removeFd(index);
		return;
	}
	state.buffer.append(tmp, n);
	parseRequest(fd, state);
}


void PollServer::run()
{
	while (true)
	{
		int ret = poll(&_fds[0], _fds.size(), -1);
		if (ret < 0)
			throw std::runtime_error("poll() failed");
		for (size_t i = 0; i < _fds.size(); ++i)
		{
			if (_fds[i].revents & POLLIN)
			{
				bool isListener = false;
				for (size_t j = 0; j < _listeners.size(); ++j)
					if (_listeners[j].getFd() == _fds[i].fd)
						isListener = true;
				if (isListener)
					handleNewConnection(i);
				else
					handleClientEvent(i);
			}
		}
	}
}
