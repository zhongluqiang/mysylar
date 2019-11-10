#include "socket.h"
#include "address.h"
#include "log.h"
#include "sylar.h"
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

Socket::ptr Socket::CreateTCP(sylar::Address::ptr address) {
    Socket::ptr sock(new Socket(address->getFamily(), TCP, 0));
    return sock;
}

Socket::ptr Socket::CreateUDP(sylar::Address::ptr address) {
    Socket::ptr sock(new Socket(address->getFamily(), UDP, 0));
    return sock;
}

Socket::ptr Socket::CreateTCPSocket() {
    Socket::ptr sock(new Socket(IPv4, TCP, 0));
    return sock;
}

Socket::ptr Socket::CreateUDPSocket() {
    Socket::ptr sock(new Socket(IPv4, UDP, 0));
    return sock;
}

Socket::Socket(int family, int type, int protocol)
    : m_sock(-1)
    , m_family(family)
    , m_type(type)
    , m_protocol(protocol)
    , m_isConnected(false) {}

Socket::~Socket() { close(); }

void Socket::newSock() {
    m_sock = socket(m_family, m_type, m_protocol);
    if (m_sock != -1) {
        initSock();
    } else {
        SYLAR_LOG_ERROR(g_logger)
            << "socket(" << m_family << ", " << m_type << ", " << m_protocol
            << ") errno=" << errno << "errstr=" << strerror(errno);
    }
}

void Socket::initSock() {
    int val = 1;
    setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int));
    if (m_type == SOCK_STREAM) {
        setsockopt(m_sock, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(int));
    }
}

bool Socket::isValid() const { return m_sock != -1; }

bool Socket::init(int sock) {
    m_sock        = sock;
    m_isConnected = true;
    initSock();
    getLocalAddress();
    getReomteAddress();
    return true;
}

Socket::ptr Socket::accept() {
    Socket::ptr sock(new Socket(m_family, m_type, m_protocol));
    int newsock = ::accept(m_sock, nullptr, nullptr);
    if (newsock == -1) {
        SYLAR_LOG_ERROR(g_logger) << "accept(" << m_sock << ") errno=" << errno
                                  << "errstr=" << strerror(errno);
        return nullptr;
    }
    if (sock->init(newsock)) {
        return sock;
    }
    return nullptr;
}

bool Socket::bind(const Address::ptr addr) {
    if (!isValid()) {
        newSock();
        if (!isValid()) {
            return false;
        }
    }

    if (addr->getFamily() != m_family) {
        SYLAR_LOG_ERROR(g_logger)
            << "bind sock.family(" << m_family << ") addr.family("
            << addr->getFamily() << ") not equal, addr=" << addr->toString();
        return false;
    }

    if (::bind(m_sock, addr->getAddr(), addr->getAddrLen())) {
        SYLAR_LOG_ERROR(g_logger)
            << "bind error errno=" << errno << "errstr=" << strerror(errno);
        return false;
    }

    getLocalAddress();
    return true;
}

bool Socket::connect(const Address::ptr addr) {
    if (!isValid()) {
        newSock();
        if (!isValid()) {
            return false;
        }
    }

    if (addr->getFamily() != m_family) {
        SYLAR_LOG_ERROR(g_logger)
            << "connect sock.family(" << m_family << ") addr.family("
            << addr->getFamily() << ") not equal, addr=" << addr->toString();
        return false;
    }

    if (::connect(m_sock, addr->getAddr(), addr->getAddrLen())) {
        SYLAR_LOG_ERROR(g_logger)
            << "sock=" << m_sock << " connect(" << addr->toString()
            << ") error errno=" << errno << "errstr=" << strerror(errno);
        close();
        return false;
    }

    m_isConnected = true;
    getReomteAddress();
    getLocalAddress();
    return true;
}

bool Socket::listen(int backlog) {
    if (!isValid()) {
        SYLAR_LOG_ERROR(g_logger) << "listen error sock=-1";
        return false;
    }

    if (::listen(m_sock, backlog)) {
        SYLAR_LOG_ERROR(g_logger)
            << "listen error errno=" << errno << "errstr=" << strerror(errno);
        return false;
    }

    return true;
}

bool Socket::close() {
    if (!m_isConnected && m_sock == -1) {
        return true;
    }

    m_isConnected = false;
    if (m_sock != -1) {
        ::close(m_sock);
        m_sock = -1;
    }

    return true;
}

int Socket::send(const void *buffer, size_t length, int flags) {
    if (isConnected()) {
        return ::send(m_sock, buffer, length, flags);
    }
    return -1;
}

int Socket::sendTo(const void *buffer, size_t length, const Address::ptr to,
                   int flags) {
    if (isConnected()) {
        return ::sendto(m_sock, buffer, length, flags, to->getAddr(),
                        to->getAddrLen());
    }
    return -1;
}

int Socket::recv(void *buffer, size_t length, int flags) {
    if (isConnected()) {
        return ::recv(m_sock, buffer, length, flags);
    }
    return -1;
}

int Socket::recvFrom(void *buffer, size_t length, Address::ptr from,
                     int flags) {
    if (isConnected()) {
        socklen_t len = from->getAddrLen();
        return ::recvfrom(m_sock, buffer, length, flags, from->getAddr(), &len);
    }
    return -1;
}

Address::ptr Socket::getReomteAddress() {
    if (m_remoteAddress) {
        return m_remoteAddress;
    }

    Address::ptr result;
    switch (m_family) {
    case AF_INET:
        result.reset(new IPV4Address());
        break;
    case AF_INET6:
    case AF_UNIX:
    default:
        SYLAR_LOG_ERROR(g_logger) << "addr type not supported";
        return nullptr;
    }

    socklen_t addrlen = result->getAddrLen();
    if (getpeername(m_sock, result->getAddr(), &addrlen)) {
        SYLAR_LOG_ERROR(g_logger)
            << "getpeername error sock=" << m_sock << "errno=" << errno
            << "errstr=" << strerror(errno);
        return nullptr;
    }

    m_remoteAddress = result;
    return m_remoteAddress;
}

Address::ptr Socket::getLocalAddress() {
    if (m_localAddress) {
        return m_localAddress;
    }

    Address::ptr result;
    switch (m_family) {
    case AF_INET:
        result.reset(new IPV4Address());
        break;
    case AF_INET6:
    case AF_UNIX:
    default:
        SYLAR_LOG_ERROR(g_logger) << "addr type not supported";
        return nullptr;
    }

    socklen_t addrlen = result->getAddrLen();
    if (getsockname(m_sock, result->getAddr(), &addrlen)) {
        SYLAR_LOG_ERROR(g_logger)
            << "getsockname error sock=" << m_sock << "errno=" << errno
            << "errstr=" << strerror(errno);
        return nullptr;
    }

    m_localAddress = result;
    return m_localAddress;
}

int Socket::getError() {
    int error     = 0;
    socklen_t len = sizeof(error);

    getsockopt(m_sock, SOL_SOCKET, SO_ERROR, &error, &len);
    return error;
}

std::ostream &Socket::dump(std::ostream &os) const {
    os << "[Socket sock=" << m_sock << " is connected=" << m_isConnected
       << " family=" << m_family << " type=" << m_type
       << " protocol=" << m_protocol;
    if (m_localAddress) {
        os << " local_address=" << m_localAddress->toString();
    }
    if (m_remoteAddress) {
        os << " remote_address=" << m_remoteAddress->toString();
    }
    os << "]";
    return os;
}

std::ostream &operator<<(std::ostream &os, const Socket &sock) {
    return sock.dump(os);
}

} // namespace sylar