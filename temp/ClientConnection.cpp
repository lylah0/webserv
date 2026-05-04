#include "ClientConnection.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <cstring>

// Helper to set nonblocking; small inline to avoid extra file
static int set_nonblocking_fd(int fd) {
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1) return -1;
	return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

ClientConnection::ClientConnection(int fd)
	: _fd(fd), _readBuf(), _writeBuf(), _writeOffset(0)
{
	// make sure socket is nonblocking for poll-based loop
	if (set_nonblocking_fd(_fd) < 0) {
	    // nonfatal here; caller may handle errors. Print for debugging.
	    perror("set_nonblocking_fd");
	}
}

ClientConnection::~ClientConnection()
{
	if (_fd >= 0) close(_fd);
}

int ClientConnection::getFd() const { return _fd; }

std::string &ClientConnection::getReadBuffer() { return _readBuf; }
const std::string &ClientConnection::getWriteBuffer() const { return _writeBuf; }
size_t ClientConnection::getWriteOffset() const { return _writeOffset; }
void ClientConnection::setWriteOffset(size_t off) { _writeOffset = off; }

void ClientConnection::prepareResponse(const std::string &resp) {
	_writeBuf = resp;
	_writeOffset = 0;
}

bool ClientConnection::handleRead() {
	char buf[4096];
	while (true) {
		ssize_t n = recv(_fd, buf, sizeof(buf), 0);
		if (n > 0) {
			_readBuf.append(buf, n);
			// keep reading until EAGAIN; caller will inspect _readBuf for completeness
			continue;
		} else if (n == 0) {
			// peer closed connection
			return false;
		} else {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				// no more data now
				break;
			}
			// unrecoverable error
			perror("recv");
			return false;
		}
	}
	return true;
}

bool ClientConnection::handleWrite() {
	if (_writeOffset >= _writeBuf.size()) {
		// nothing to send
		return true;
	}

	const char *data = _writeBuf.c_str() + _writeOffset;
	size_t remaining = _writeBuf.size() - _writeOffset;

	while (remaining > 0) {
		ssize_t n = send(_fd, data, remaining, 0);
		if (n > 0) {
			_writeOffset += n;
			data += n;
			remaining -= n;
			continue;
		} else {
			// forbidden to check for errno instead of using poll
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				// socket temporarily not writable
				break;
			}
			// unrecoverable error
			perror("send");
			return false;
		}
	}

	return true;
}
