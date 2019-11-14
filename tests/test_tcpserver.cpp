#include "sylar/log.h"
#include "sylar/sylar.h"
#include "sylar/tcp_server.h"

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void run(void *arg) {
    auto addr = sylar::IPAddress::Create("127.0.0.1");
    if (addr) {
        addr->setPort(8080);
        SYLAR_LOG_INFO(g_logger) << addr->toString();
    } else {
        SYLAR_LOG_ERROR(g_logger) << "sylar::IPAddress::Create failed";
        return;
    }

    sylar::TcpServer::ptr server(new sylar::TcpServer);
    if (!server->bind(addr)) {
        SYLAR_LOG_ERROR(g_logger) << "sylar::TcpServer::bind failed";
        return;
    }

    server->start();
}

int main() {
    sylar::Scheduler sc;
    sc.schedule(&run);
    sc.start();

    while (1)
        sleep(1);
    return 0;
}