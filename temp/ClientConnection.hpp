#ifndef CLIENTCONNECTION_HPP
#define CLIENTCONNECTION_HPP

#include <string>

class ClientConnection {
public:
    ClientConnection(int fd);
    ~ClientConnection();

    bool handleRead();
    bool handleWrite();

    int getFd() const;
    void prepareResponse(const std::string &resp);
    std::string &getReadBuffer();
    const std::string &getWriteBuffer() const;
    size_t getWriteOffset() const;
    void setWriteOffset(size_t off);

private:
    int _fd;
    std::string _readBuf;
    std::string _writeBuf;
    size_t _writeOffset;
};

#endif
