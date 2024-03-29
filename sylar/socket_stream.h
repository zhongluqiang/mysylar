#ifndef MYSYLAR_SOCKET_STREAM_H
#define MYSYLAR_SOCKET_STREAM_H

#include "socket.h"
#include "stream.h"

namespace sylar {

class SocketStream : public Stream {
public:
    typedef std::shared_ptr<SocketStream> ptr;

    SocketStream(Socket::ptr sock, bool owner = true);
    ~SocketStream();

    virtual int read(void *buffer, size_t length) override;
    virtual int read(ByteArray::ptr ba, size_t length) override;

    virtual int write(const void *buffer, size_t length) override;
    virtual int write(ByteArray::ptr ba, size_t length) override;

    virtual void close() override;

    Socket::ptr getSocket() const { return m_socket; }
    bool isConnected() const;

private:
    Socket::ptr m_socket;
    bool m_owner;
};

} // namespace sylar

#endif