#include "http_server.h"
#include "sylar/log.h"

namespace sylar {
namespace http {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

HttpServer::HttpServer(bool keepalive)
    : m_isKeepalive(keepalive) {}

void HttpServer::handleClient(Socket::ptr client) {
    HttpSession::ptr session(new HttpSession(client));
    do {
        HttpRequest::ptr req = session->recvRequest();
        if (!req) {
            SYLAR_LOG_WARN(g_logger)
                << "recv http request fail, errno=" << errno
                << " errstr=" << strerror(errno) << " client: " << *client
                << " keepalive=" << m_isKeepalive;
            break;
        }
        HttpResponse::ptr rsp(new HttpResponse(
            req->getVersion(), req->isClose() || !m_isKeepalive));
        rsp->setHeader("Server", getName());
        rsp->setBody("hello mysylar");
        session->sendResponse(rsp);
        if (!m_isKeepalive || req->isClose()) {
            break;
        }
    } while (true);
    session->close();
}
} // namespace http
} // namespace sylar