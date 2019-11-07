#include "hook_sys_call.h"
#include "fiber.h"
#include "log.h"
#include "scheduler.h"
#include "sylar.h"
#include "timer.h"
#include <dlfcn.h>
#include <unistd.h>

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

namespace sylar {

static thread_local bool t_hook_sys_enable = false;

bool is_enable_hook_sys_call() { return t_hook_sys_enable; }

void enable_hook_sys_call() { t_hook_sys_enable = true; }

static void hook_init() {
    // g_logger->setLevel(sylar::LogLevel::UNKNOWN); //调试时打开
}
struct _HookIniter {
    _HookIniter() {
        hook_init();
        // todo：设置更多hook之前需要初始化的参数，如tcp连接超时，读写超时
    }
};
static _HookIniter s_hook_initer;

} // namespace sylar

extern "C" {

typedef unsigned int (*sleep_func_t)(unsigned int seconds);
typedef int (*usleep_func_t)(useconds_t usec);

static sleep_func_t g_sys_sleep_func   = nullptr;
static usleep_func_t g_sys_usleep_func = nullptr;

#define HOOK_SYS_FUNC(name)                                                    \
    if (!g_sys_##name##_func) {                                                \
        g_sys_##name##_func = (name##_func_t)dlsym(RTLD_NEXT, #name);          \
    }

unsigned int sleep(unsigned int seconds) {
    SYLAR_LOG_DEBUG(g_logger) << "enter hooked sleep";
    HOOK_SYS_FUNC(sleep);
    if (!sylar::is_enable_hook_sys_call()) {
        return g_sys_sleep_func(seconds);
    }

    sylar::Fiber::ptr self = sylar::Fiber::GetThis();
    sylar::Scheduler *psc  = sylar::Scheduler::getThis();
    SYLAR_ASSERT(self);
    SYLAR_ASSERT(psc);

    sylar::Timer::ptr timer(new sylar::Timer(seconds * 1000, self));
    psc->timer_schedule(timer, EPOLL_CTL_ADD);
    self->yield();
    psc->timer_schedule(timer, EPOLL_CTL_DEL);
    return 0;
}

int usleep(useconds_t usec) {
    SYLAR_LOG_DEBUG(g_logger) << "enter hooked usleep";
    HOOK_SYS_FUNC(usleep);
    if (!sylar::is_enable_hook_sys_call()) {
        return g_sys_usleep_func(usec);
    }
    sylar::Fiber::ptr self = sylar::Fiber::GetThis();
    sylar::Scheduler *psc  = sylar::Scheduler::getThis();
    SYLAR_ASSERT(self);
    SYLAR_ASSERT(psc);

    sylar::Timer::ptr timer(new sylar::Timer(usec / 1000, self));
    psc->timer_schedule(timer, EPOLL_CTL_ADD);
    self->yield();
    psc->timer_schedule(timer, EPOLL_CTL_DEL);
    return 0;
}

} // end extern "C"
