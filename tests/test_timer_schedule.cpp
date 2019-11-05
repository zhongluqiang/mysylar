#include "sylar/sylar.h"
#include "sylar/timer.h"

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

//使用定时器调度功能实现hook系统函数sleep()

void test_hook_sleep(void *arg) {
    SYLAR_LOG_INFO(g_logger) << "test_hook_sleep begin";

    sylar::Fiber::ptr self = sylar::Fiber::GetThis();
    sylar::Scheduler *psc  = sylar::Scheduler::getThis();
    SYLAR_ASSERT(self);
    SYLAR_ASSERT(psc);

    //添加定时器任务，2秒后再调度当前协程
    sylar::Timer::ptr timer(new sylar::Timer(2000, self));
    psc->timer_schedule(timer, EPOLL_CTL_ADD);
    //当前协程让出CPU执行时间，但并未退出，2秒后定时器超时时，当前协程还会再运行
    self->yield();
    //删除定时器任务，否则IO调度还在，调度线程永远不会结束
    psc->timer_schedule(timer, EPOLL_CTL_DEL);

    //再来一次
    sylar::Timer::ptr timer2(new sylar::Timer(2000, self));
    psc->timer_schedule(timer2, EPOLL_CTL_ADD);
    self->yield();
    psc->timer_schedule(timer2, EPOLL_CTL_DEL);

    SYLAR_LOG_INFO(g_logger) << "test_hook_sleep end";
}

void test_sys_sleep(void *arg) {
    SYLAR_LOG_INFO(g_logger) << "test_sys_sleep begin";
    sleep(3);
    SYLAR_LOG_INFO(g_logger) << "test_sys_sleep end";
}

int main() {
    sylar::Scheduler sc;

    sc.schedule(&test_hook_sleep);
    sc.schedule(&test_sys_sleep);

    sc.start();

    //非强制结束的情况下，调度器停止的条件是无待执行的协程和无待调度的IO协程，否则调度器
    //不会停止，这使得添加的timer必须删除，否则调度器永远不会结束，待优化
    sc.stop();

    return 0;
}