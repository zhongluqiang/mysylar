#ifndef MYSYLAR_HTTP_SERVER_H
#define MYSYLAR_HTTP_SERVER_H

#include "http_session.h"
#include "sylar/tcp_server.h"

namespace sylar {
namespace http {

class HttpServer : public TcpServer {
public:
    typedef std::shared_ptr<HttpServer> ptr;

    HttpServer(bool keepalive = false);

protected:
    virtual void handleClient(Socket::ptr client) override;

private:
    // 是否支持长连接
    bool m_isKeepalive;
};
} // namespace http
} // namespace sylar

#endif