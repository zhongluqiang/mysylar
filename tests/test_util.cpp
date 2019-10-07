#include "sylar/sylar.h"

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void func3() { SYLAR_LOG_INFO(g_logger) << sylar::BacktraceToString(10); }

void func2() { func3(); }

void func1() { func2(); }

void test_backtrace() { func1(); }

void test_macro() { SYLAR_ASSERT(0); }

int main() {
    test_backtrace();
    test_macro();
    return 0;
}