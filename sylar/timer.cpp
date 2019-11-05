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

    m_fd   = fd;
    m_task = task;
}

} // end namespace sylar