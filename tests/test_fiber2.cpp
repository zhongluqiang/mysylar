#include "sylar/fiber.h"
#include "sylar/sylar.h"
#include <string>
#include <vector>

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void run_in_fiber2() {
    SYLAR_LOG_INFO(g_logger) << "run_in_fiber2 begin";

    // SYLAR_LOG_INFO(g_logger) << "before run_in_fiber2 yield";
    // sylar::Fiber::GetThis()->yield();
    // SYLAR_LOG_INFO(g_logger) << "after run_in_fiber2 yield";

    SYLAR_LOG_INFO(g_logger) << "run_in_fiber2 end";
}

void run_in_fiber() {
    SYLAR_LOG_INFO(g_logger) << "run_in_fiber begin";

    sylar::Fiber::ptr fiber(new sylar::Fiber(run_in_fiber2));
    SYLAR_LOG_INFO(g_logger) << "fiber = " << fiber;

    fiber->resume();

    // SYLAR_LOG_INFO(g_logger) << "before run_in_fiber yield";
    // sylar::Fiber::GetThis()->yield();
    // SYLAR_LOG_INFO(g_logger) << "after run_in_fiber yield";

    SYLAR_LOG_INFO(g_logger) << "run_in_fiber end";
}

int main() {
    SYLAR_LOG_INFO(g_logger) << "main begin";

    sylar::Fiber::GetThis();

    sylar::Fiber::ptr fiber(new sylar::Fiber(run_in_fiber));
    SYLAR_LOG_INFO(g_logger) << "fiber = " << fiber;
    fiber->resume();

    SYLAR_LOG_INFO(g_logger) << "main end";
    return 0;
}