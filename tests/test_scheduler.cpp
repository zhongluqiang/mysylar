#include "sylar/sylar.h"

// 单线程协程调度器，调度器有自己的线程，不允许跨线程进行协程调度

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void test_fiber1(void *arg) {
    SYLAR_LOG_INFO(g_logger) << "test_fiber1 begin, arg=" << arg;
    /* 协程主动让出执行权给下一个要调度的协程执行，当前协程停止执行，调度器将自动将当前
     * 协程重新添加到任务队列的尾部，等待下一次调度
     */
    sylar::Fiber::GetThis()->yield();
    SYLAR_LOG_INFO(g_logger) << "test_fiber1 end";
}

void test_fiber2(void *arg) {
    SYLAR_LOG_INFO(g_logger) << "test_fiber2 begin, arg=" << arg;
    /* 由于所有协程在一个线程里交替执行，所以任何一个协程阻塞时都会影响整个协程调度，这里
     * 睡眠的3秒钟之内调度器不会调度新的协程，对系统调用进行hook之后可以改变这种情况
     */
    sleep(3);
    SYLAR_LOG_INFO(g_logger) << "test_fiber2 end";
}
void test_fiber3(void *arg) {
    SYLAR_LOG_INFO(g_logger) << "test_fiber3 begin, arg=" << arg;
    SYLAR_LOG_INFO(g_logger) << "test_fiber3 end";
}
void test_fiber4(void *arg) {
    SYLAR_LOG_INFO(g_logger) << "test_fiber4 begin, arg=" << arg;
    /* 最后一个任务睡眠10秒，用于演示调度器强制停止的行为
     */
    sleep(10);
    SYLAR_LOG_INFO(g_logger) << "test_fiber4 end";
}

int main() {
    SYLAR_LOG_INFO(g_logger) << "main begin";

    sylar::Scheduler sc;

    /*添加调度任务，使用函数作为调度对象*/
    sc.schedule(&test_fiber1, (void *)0x123);
    sc.schedule(&test_fiber2, (void *)0x456);

    /*添加调度任务，使用Fiber类作为调度对象*/
    sylar::Fiber::ptr fiber(new sylar::Fiber(&test_fiber3, (void *)0x789));
    sc.schedule(fiber);

    /*开始调度，所有调度任务按添加的先后顺序执行，
     *主动让出执行权的协程会被重新追加到任务队列的尾部，最终也会被执行*/
    sc.start();

    /*批量添加协程对象作为调度任务，可在创建协程时指定执行参数*/
    std::vector<sylar::Fiber::ptr> fibers;
    fibers.push_back(
        sylar::Fiber::ptr(new sylar::Fiber(&test_fiber3, (void *)0x11111)));
    fibers.push_back(
        sylar::Fiber::ptr(new sylar::Fiber(&test_fiber3, (void *)0x22222)));
    sc.schedule(fibers.begin(), fibers.end());

    /*批量添加函数指针对象作为调度任务，执行参数只能为nullptr*/
    std::vector<std::function<void(void *)>> functions{test_fiber3,
                                                       test_fiber3};
    sc.schedule(functions.begin(), functions.end());

    /* 睡眠10秒，等所有任务都执行完，当所有任务都执行完后，调度器执行idle任务，该任务
     * 默认将调度线程挂起，直到有新任务被添加进来*/
    sleep(5);

    /* 只要调度器未停止，就可以添加调度任务 */
    sc.schedule(&test_fiber4);

    /* 停止调度，默认参数为false，调度器会等所有调度任务执行结束之后再返回，
     * 当参数传true时，表示强制停止，调度器会强行结束当前所有任务并立即返回
     */
    sc.stop();
    /* 强制停止调度可能导致协程分配的资源未释放问题，强烈不建议使用 */
    // sleep(1);
    // sc.stop(true);

    SYLAR_LOG_INFO(g_logger) << "main end";
    return 0;
}