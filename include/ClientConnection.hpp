/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ClientConnection.hpp                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lylrandr <lylrandr@student.42lausanne.ch>  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/13 18:27:14 by lylrandr          #+#    #+#             */
/*   Updated: 2026/06/10 11:42:56 by lylrandr         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENTCONNECTION_HPP
#define CLIENTCONNECTION_HPP

# include <string>
# include <map>
# include <unistd.h>
# include <sys/socket.h>
#  include <fstream>
#  include <sstream>
# include <deque>
# include <cstdlib>
# include <stdint.h>
# include <cstring>
#  include "ServerConfig.hpp"
# include "HttpResponse.hpp"
# include "HttpResponse.hpp"

class ClientConnection {
private :
    int             _fd;
    std::string     _readBuffer;
    bool            _headersParsed;
    size_t          _expectedLength;
    size_t          _headerEnd;

    std::deque<std::string> _responseQueue;
    size_t      _writeOffset;
    uint64_t    _requestId;

    ClientConnection(ClientConnection const &src);
    ClientConnection& operator=(ClientConnection const &rhs);

public :
    ClientConnection(int fd);
    ~ClientConnection();

    std::string const& getReadBuffer() const;
    std::string& getMutableReadBuffer();
    void eraseReadBytes(size_t n);
    void appendToReadBuffer(const char* data, size_t len);
    void popReadBytes(size_t n);

    void enqueueResponse(const std::string &resp);
    void replaceReadBuffer(const std::string& s);
    bool hasPendingResponses() const;
    const std::string &currentResponse() const;
    void popResponse();

    bool handleWrite();
    bool writeComplete() const;
    void clearWrite();
    void prepResponse(const HttpResponse &response);
    bool handleRead();
    bool isRequestComplete() const;
    void resetReadState();
    void reset();
    void setRequestId(uint64_t id);
    uint64_t getRequestId() const;
    std::string getBuffer() const;
    size_t getOffset() const;
    int getFd() const;
	void					clearReadBuffer();
};

#endif
