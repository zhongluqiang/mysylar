#include "tcp_server.h"
#include "log.h"
#include <functional>
#include <string.h>

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("tcpserver");

TcpServer::TcpServer()
    : m_name("sylar/1.0.0")
    , m_isStop(true) {
    // g_logger->setLevel(LogLevel::UNKNOWN); //调试时打开
}

TcpServer::~TcpServer() { m_sock->close(); }

bool TcpServer::bind(sylar::Address::ptr addr) {
    m_sock = Socket::CreateTCP(addr);
    if (!m_sock->bind(addr)) {
        SYLAR_LOG_ERROR(g_logger)
            << "bind fail errno=" << errno << "errstr=" << strerror(errno)
            << "addr=[" << addr->toString() << "]";
        return false;
    }

    if (!m_sock->listen()) {
        SYLAR_LOG_ERROR(g_logger) << "listen fail errno=" << errno
                                  << "addr=" << addr->toString() << "]";
        return false;
    }

    return true;
}

bool TcpServer::start() {
    if (!m_isStop) {
        return true;
    }
    m_isStop = false;
    m_acceptWorker.schedule(
        std::bind(&TcpServer::startAccept, shared_from_this(), m_sock));
    m_acceptWorker.start();
    m_worker.start();
    return true;
}

void TcpServer::stop() {
    m_isStop = true;
    m_acceptWorker.stop(true);
    m_worker.stop(true);
}

void TcpServer::startAccept(Socket::ptr sock) {
    while (!m_isStop) {
        SYLAR_LOG_DEBUG(g_logger) << "start accept:" << *sock;
        Socket::ptr client = sock->accept();
        SYLAR_LOG_DEBUG(g_logger) << "accepted:" << *client;
        if (client) {
            m_worker.schedule(std::bind(&TcpServer::handleClient,
                                        shared_from_this(), client));
        } else {
            SYLAR_LOG_ERROR(g_logger)
                << "accept errno=" << errno << "errstr=" << strerror(errno);
        }
    }
}
void TcpServer::handleClient(Socket::ptr client) {
    SYLAR_LOG_INFO(g_logger) << "handleClient:" << *client;
    client->close();
}

} // namespace sylar