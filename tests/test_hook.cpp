#include "sylar/hook_sys_call.h"
#include "sylar/sylar.h"

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void test_hook_sleep(void *arg) {
    SYLAR_LOG_INFO(g_logger) << "test_hook_sleep begin";
    sleep(2);
    SYLAR_LOG_INFO(g_logger) << "test_hook_sleep end";
}

int main() {
    SYLAR_LOG_INFO(g_logger) << "main begin";
    sylar::Scheduler sc;
    sc.schedule(&test_hook_sleep);
    sc.schedule(&test_hook_sleep);
    sc.schedule(&test_hook_sleep);
    sc.start();
    sc.stop();
    SYLAR_LOG_INFO(g_logger) << "main end";
    return 0;
}