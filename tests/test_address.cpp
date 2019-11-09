#include "sylar/address.h"
#include "sylar/log.h"

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void test_lookup() {
    std::vector<sylar::Address::ptr> addrs;
    SYLAR_LOG_INFO(g_logger) << "begin";
    bool v = sylar::Address::Lookup(addrs, "www.baidu.com", AF_INET);
    SYLAR_LOG_INFO(g_logger) << "end";
    if (!v) {
        SYLAR_LOG_ERROR(g_logger) << "lookup fail";
        return;
    }

    for (size_t i = 0; i < addrs.size(); ++i) {
        SYLAR_LOG_INFO(g_logger) << i << " - " << addrs[i]->toString();
    }
}

void test_ipv4() {
    auto addr = sylar::IPAddress::Create("192.168.0.111");
    // addr->setPort(80);
    if (addr) {
        SYLAR_LOG_INFO(g_logger) << addr->toString();
        SYLAR_LOG_INFO(g_logger) << addr->broadcastAddress(24)->toString();
        SYLAR_LOG_INFO(g_logger) << addr->subnetMask(24)->toString();
        SYLAR_LOG_INFO(g_logger) << addr->networkAddress(24)->toString();
    }
}

int main() {
    test_lookup();
    test_ipv4();

    return 0;
}