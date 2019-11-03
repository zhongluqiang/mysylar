#include "timer.h"
#include "log.h"
#include "macro.h"
#include "scheduler.h"
#include <sys/timerfd.h>

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

namespace sylar {

void Timer::initTimer(long timeout, ScheduleTask task) {
    int fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    SYLAR_ASSERT(fd > 0);

    struct itimerspec ts = {0};
    ts.it_value.tv_sec   = timeout / 1000;
    ts.it_value.tv_nsec  = (timeout % 1000) * 1000 * 1000;

    int rt = timerfd_settime(fd, 0, &ts, nullptr);
    SYLAR_ASSERT(rt == 0);

    m_context.fd   = fd;
    m_context.task = task;
}

void Timer::timer_proc(void *arg) {
    SYLAR_ASSERT(arg != nullptr);

    TimerContext *pContext = (TimerContext *)arg;
    int timerfd            = pContext->fd;
    SYLAR_ASSERT(timerfd > 0);

    // uint64_t buf;
    // while (read(timerfd, &buf, sizeof(uint64_t)) > 0)
    //     ;

    if (pContext->task.cb) {
        pContext->task.cb(pContext->task.arg);
    } else if (pContext->task.fiber) {
        pContext->task.fiber->resume();
    }

    close(timerfd);
    delete pContext;
}

} // end namespace sylar