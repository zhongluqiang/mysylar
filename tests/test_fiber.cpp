#include "sylar/fiber.h"
#include "sylar/sylar.h"
#include <string>
#include <vector>

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void run_in_fiber2(void *arg) {
    SYLAR_LOG_INFO(g_logger) << "run_in_fiber2 begin, arg=" << arg;
    SYLAR_LOG_INFO(g_logger) << "run_in_fiber2 end";
}

void run_in_fiber(void *arg) {
    SYLAR_LOG_INFO(g_logger) << "run_in_fiber begin, arg=" << arg;

    SYLAR_LOG_INFO(g_logger) << "before run_in_fiber yield";
    sylar::Fiber::GetThis()->yield();
    SYLAR_LOG_INFO(g_logger) << "after run_in_fiber yield";

    SYLAR_LOG_INFO(g_logger) << "run_in_fiber end";
    // fiber结束之后会自动yield，所以不需要手动执行yield
}

void test_fiber() {
    SYLAR_LOG_INFO(g_logger) << "test_fiber begin";

    sylar::Fiber::GetThis();
    sylar::Fiber::ptr fiber(new sylar::Fiber(run_in_fiber, (void *)0x12345));
    SYLAR_LOG_INFO(g_logger) << "use_coun:" << fiber.use_count(); // 1
    SYLAR_LOG_INFO(g_logger) << "before test_fiber resume";
    fiber->resume();
    SYLAR_LOG_INFO(g_logger) << "after test_fiber resume";

    // 关于fiber智能指针的引用计数为3的说明：
    //一份在当前函数的fiber指针，一份在MainFunc的cur指针
    //还有一份在在run_in_fiber的GetThis()结果的临时变量里
    SYLAR_LOG_INFO(g_logger) << "use_coun:" << fiber.use_count(); // 3

    SYLAR_LOG_INFO(g_logger)
        << "fiber status: " << fiber->getState(); // FIBER_PENDING

    SYLAR_LOG_INFO(g_logger) << "before test_fiber resume again";
    fiber->resume();
    SYLAR_LOG_INFO(g_logger) << "after test_fiber resume again";
    SYLAR_LOG_INFO(g_logger) << "use_coun:" << fiber.use_count(); // 1
    SYLAR_LOG_INFO(g_logger)
        << "fiber status: " << fiber->getState(); // FIBER_TERMINATED

    /* 上一个协程结束之后，复用其栈空间再创建一个新协程 */
    fiber->reset(run_in_fiber2, (void *)0x67890);
    fiber->resume();

    SYLAR_LOG_INFO(g_logger) << "use_coun:" << fiber.use_count(); // 1
    SYLAR_LOG_INFO(g_logger) << "test_fiber end";
}

int main() {
    SYLAR_LOG_INFO(g_logger) << "main begin";
    // test_fiber();
    sylar::Thread::SetName("main");
    std::vector<sylar::Thread::ptr> thrs;
    for (int i = 0; i < 2; i++) {
        thrs.push_back(sylar::Thread::ptr(
            new sylar::Thread(&test_fiber, "name_" + std::to_string(i))));
    }

    for (auto i : thrs) {
        i->join();
    }
    SYLAR_LOG_INFO(g_logger) << "main end";
    return 0;
}