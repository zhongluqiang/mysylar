#ifndef MYSYLAR_TIMER_H
#define MYSYLAR_TIMER_H

#include "fiber.h"
#include "thread.h"
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <unistd.h>

namespace sylar {

/*调度任务，可以是函数，也可以是Fiber类对象，如果是函数，则需要传递参数，Fiber类
 *对象则不需要
 */
struct ScheduleTask {
    Fiber::ptr fiber;
    std::function<void(void *)> cb;
    void *arg;

    ScheduleTask(Fiber::ptr f, void *_arg = nullptr) { fiber = f; }
    ScheduleTask(std::function<void(void *)> f, void *_arg = nullptr) {
        cb  = f;
        arg = _arg;
    }
    /*SchdeuleTask有个二义性问题，即构造函数传参数nullptr进来时可以同时匹配到以上两
     *个构造函数，为避免编译提示二义性错误，增加一个普通指针的构造函数，只为了消除编译
     *错误，一般不用
     */
    ScheduleTask(void *ptr = nullptr, void *_arg = nullptr) {
        fiber = nullptr;
        cb    = nullptr;
    }

    void reset() {
        fiber = nullptr;
        cb    = nullptr;
    }
};

class Timer {
public:
    typedef std::shared_ptr<Timer> ptr;

    struct TimerContext {
        int fd;
        ScheduleTask task;
    };

public:
    /* 初始化定时器
     * timeout: 超时时间，单位ms
     * fc: 超时时的执行对象
     */
    template <class FiberOrCb> Timer(long timeout, FiberOrCb cb) {
        ScheduleTask task(cb);
        if (task.fiber || task.cb) {
            initTimer(timeout, task);
        }
    }

    ~Timer() {}

    TimerContext getcontext() { return m_context; }

public:
    static void timer_proc(void *arg);

private:
    void initTimer(long timeout, ScheduleTask task);

    //禁止拷贝
    Timer(const Timer &)  = delete;
    Timer(const Timer &&) = delete;
    Timer operator=(const Timer &) = delete;

private:
    TimerContext m_context;
};

} // end namespace sylar

#endif