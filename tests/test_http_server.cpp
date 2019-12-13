#include "sylar/http/http_server.h"

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void run(void *) {
    sylar::http::HttpServer::ptr server(new sylar::http::HttpServer);
    auto addr = sylar::IPAddress::Create("0.0.0.0");
    addr->setPort(8020);
    while (!server->bind(addr)) {
        sleep(2);
    }
    SYLAR_LOG_INFO(g_logger) << "Server start...";
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