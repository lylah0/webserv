/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ClientConnection.cpp                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lylrandr <lylrandr@student.42lausanne.ch>  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/13 19:15:42 by lylrandr          #+#    #+#             */
/*   Updated: 2026/06/24 01:21:05 by lylrandr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ClientConnection.hpp"
#include <iostream>
#include <sstream>
#include <cstring>

ClientConnection::ClientConnection(int fd)
: _fd(fd), _headersParsed(false), _expectedLength(0), _headerEnd(0),
  _writeOffset(0), _requestId(0)
{
}

ClientConnection::~ClientConnection(){
    close(_fd);
}

std::string const& ClientConnection::getReadBuffer() const {
    return _readBuffer;
}

std::string& ClientConnection::getMutableReadBuffer()
{
    return _readBuffer;
}

void ClientConnection::eraseReadBytes(size_t n)
{
    if (n > 0 && n <= _readBuffer.size())
        _readBuffer.erase(0, n);
}

void ClientConnection::appendToReadBuffer(const char* data, size_t len)
{
    //std::cerr << "\nERROR : append error found here\n" << std::endl;
    _readBuffer.append(data, len);
}

void ClientConnection::replaceReadBuffer(const std::string& s)
{
    _readBuffer = s;
    _headersParsed = true;

    size_t pos = _readBuffer.find("\r\n\r\n");
    if (pos == std::string::npos)
    {
        _headerEnd = 0;
        _expectedLength = 0;
        return;
    }
    _headerEnd = pos + 4;
    _expectedLength = _readBuffer.size() - _headerEnd;
}

void ClientConnection::popReadBytes(size_t n) {
    if (n >= _readBuffer.size()) {
        _readBuffer.clear();
    } else {
        _readBuffer.erase(0, n);
    }
}

void ClientConnection::enqueueResponse(const std::string &resp) {
    bool wasEmpty = _responseQueue.empty();
    _responseQueue.push_back(resp);
    if (wasEmpty)
        _writeOffset = 0;
}

bool ClientConnection::hasPendingResponses() const {
    return !_responseQueue.empty();
}

const std::string &ClientConnection::currentResponse() const {
    return _responseQueue.front();
}

void ClientConnection::popResponse() {
    if (!_responseQueue.empty())
        _responseQueue.pop_front();
    _writeOffset = 0;
}

bool ClientConnection::handleWrite() {
    if (_responseQueue.empty()) {
        return true;
    }
    const std::string &buf = _responseQueue.front();
    const char *data = buf.c_str() + _writeOffset;
    size_t rest = buf.size() - _writeOffset;
    /*if (_writeOffset == 0) { // only check at the start of sending
        if (buf.find("404 Not Found") != std::string::npos ||
            buf.find("HTTP/1.1 404") != std::string::npos)
        {
            std::cerr << "[DEBUG 404] WIRE OUT:\n";
            std::cerr << std::string(data, rest) << "\n";
        }
        else if (buf.find("504 Gateway Timeout") != std::string::npos ||
            buf.find("HTTP/1.1 504") != std::string::npos)
        {
            std::cerr << "[DEBUG 504] WIRE OUT:\n";
            std::cerr << std::string(data, rest) << "\n";
        }
    }*/
    ssize_t sent = send(_fd, data, rest, 0);
    if (sent > 0)
    {
        _writeOffset += static_cast<size_t>(sent);
        return true;
    }
    if (sent == 0)
        return false;
    return true;
}

bool ClientConnection::writeComplete() const {
    if (_responseQueue.empty()) return true;
    return _writeOffset >= _responseQueue.front().size();
}

void ClientConnection::clearWrite() {
    _responseQueue.clear();
    _writeOffset = 0;
}

void ClientConnection::prepResponse(const HttpResponse &response)
{
    std::ostringstream out;
    out << "HTTP/1.1 " << response.statusCode << " " << response.statusMessage << "\r\n";
    for (std::map<std::string, std::string>::const_iterator it = response.headers.begin();
            it != response.headers.end(); it++)
        out << it->first << ": " << it->second << "\r\n";
    out << "\r\n";
    out << response.body;
    //if (response.statusCode == 404)
        //std::cerr << "[DEBUG] PREPRESPONSE ENQUEUED RESPONSE : \n" << out.str() << std::endl;
    //std::cerr << "[DEBUG] printing headers : HTTP/1.1 " << response.statusCode << std::endl;
    enqueueResponse(out.str());
}

bool ClientConnection::handleRead() {
    char buf[4096];
    ssize_t n = recv(_fd, buf, sizeof(buf), 0);
    if (n <= 0)
        return false;
    _readBuffer.append(buf, static_cast<size_t>(n));
    if (!_headersParsed) {
        size_t pos = _readBuffer.find("\r\n\r\n");
        if (pos != std::string::npos) {
            _headersParsed = true;
            _headerEnd = pos + 4;
            std::string headerPart = _readBuffer.substr(0, pos + 4);
            size_t clPos = headerPart.find("Content-Length:");
            if (clPos != std::string::npos) {
                clPos += 15;
                while (clPos < headerPart.size() && headerPart[clPos] == ' ')
                    ++clPos;
                _expectedLength = std::atoi(headerPart.c_str() + clPos);
            } else {
                _expectedLength = 0;
            }
        }
    }
    return true;
}

bool ClientConnection::isRequestComplete() const {
    if (!_headersParsed)
        return false;
    if (_expectedLength == 0)
        return true;
    size_t bodySize = _readBuffer.size() - _headerEnd;
    return bodySize >= _expectedLength;
}

void ClientConnection::resetReadState() {
    _readBuffer.clear();
    _headersParsed = false;
    _expectedLength = 0;
    _headerEnd = 0;
}

void ClientConnection::reset() {
    resetReadState();
    clearWrite();
    _requestId = 0;
}

void ClientConnection::setRequestId(uint64_t id) {
    _requestId = id;
}
uint64_t ClientConnection::getRequestId() const {
    return _requestId;
}

std::string ClientConnection::getBuffer() const {
    if (_responseQueue.empty()) return std::string();
    return _responseQueue.front();
}

size_t ClientConnection::getOffset() const {
    return _writeOffset;
}

int ClientConnection::getFd() const {
    return _fd;
}

void	ClientConnection::clearReadBuffer(){
	_readBuffer.clear();
}
