#ifndef MYSYLAR_SCHEDULER_H
#define MYSYLAR_SCHEDULER_H

#include "fiber.h"
#include "log.h"
#include "thread.h"
#include <functional>
#include <list>
#include <memory>
#include <string>
#include <sys/epoll.h>

namespace sylar {

class Scheduler;

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

/* 文件描述符上下文，在IO协程执行时作为参数传入，用于在协程执行时获取当前协程所属的文件
 * 描述符，调度器指针，以及发生的事件
 */
struct FdContext {
public:
    int fd;
    ScheduleTask task;
    std::function<void(void *)> cb;
    Scheduler *scheduler;
    epoll_event event; // events用于保存已发生的事件，data.u32用于存放原始事件
public:
    FdContext() { reset(); }
    void reset() {
        fd             = -1;
        cb             = nullptr;
        scheduler      = nullptr;
        event.events   = 0;
        event.data.u32 = 0;
    }
}; // end struct FdContext

class Scheduler {
public:
    typedef std::shared_ptr<Scheduler> ptr;
    typedef Mutex MutexType;

    Scheduler(const std::string &name = "");
    virtual ~Scheduler();

    /* 添加调度任务 */
    template <class FiberOrCb>
    void schedule(FiberOrCb fc, void *arg = nullptr) {
        bool need_tickle = false;
        ScheduleTask task(fc, arg);

        if (task.fiber || task.cb) {

            /* 如果当前是idle状态，需要唤醒idle协程，使其立即退出，进行重新调度*/
            if (m_idle) {
                need_tickle = true;
            }
            MutexType::Lock lock(m_mutex);
            m_tasks.push_back(task);
        }

        if (need_tickle) {
            tickle(); //唤醒idle协程
        }
    }

    /* 批量添加调度任务，当添加的任务为函数指针时，函数参数默认为nullptr */
    template <class InputIterator>
    void schedule(InputIterator begin, InputIterator end) {
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            while (begin != end) {
                ScheduleTask task(*begin);
                if (task.fiber || task.cb) {
                    if (m_idle && !need_tickle) {
                        need_tickle = true;
                    }
                    m_tasks.push_back(task);
                }
                ++begin;
            }
            if (need_tickle) {
                tickle();
            }
        }
    }

    /* 添加IO调度任务
     * fd: 目标文件描述符
     * op: EPOLL_CTL_ADD/EPOLL_CTL_MOD/EPOLL_CTL_DEL，添加/修改/删除事件
     * events: IO事件，参考epoll的事件枚举
     * task: 协程或回调函数
     * */
    template <class FiberOrCb>
    int io_schedule(int fd, int op, uint32_t events, FiberOrCb fc) {
        ScheduleTask task(fc);
        return io_schedule(fd, op, events, task);
    }

    int io_schedule(int fd, int op, uint32_t events, ScheduleTask task);
    // int io_schedule(int fd, int op, uint32_t events,
    //                 std::function<void(void *)> cb) {
    //     ScheduleTask task(cb);
    //     return io_schedule(fd, op, events, task);
    // }
    // int io_schedule(int fd, int op, uint32_t events, Fiber::ptr fiber) {
    //     ScheduleTask task(fiber);
    //     return io_schedule(fd, op, events, task);
    // }

    /* 启动协程调度器 */
    void start();

    /* 停止协程调度器，默认等所有协程执行完之后再返回 */
    void stop(bool force = false);

    const std::string &getName() const { return m_name; }

protected:
    /* 通知协程调度器有任务了 */
    void tickle();
    /* 协程调度函数 */
    void run();
    /* 无任务调度时执行idle协程 */
    void idle();

private:
    void resizeFdContext(size_t size);

private:
    std::string m_name;              //协程调度器名称
    MutexType m_mutex;               //互斥锁
    std::list<ScheduleTask> m_tasks; //任务队列
    Thread::ptr m_thread;            //调度线程
    bool m_idle = false;             //调度线程是否idle

    /* 使用pipe配合epoll实现协程调度 */
    int m_epfd;         // epoll句柄
    int m_tickleFds[2]; // pipe句柄， [0]读句柄，[1]写句柄

    /* 增加一个停止标志，解决stop()的tickle有可能被忽略的问题 */
    bool m_stop = false;

    /* 文件描述符上下文集合，用于IO事件调度*/
    std::vector<FdContext *> m_fdContexts;
}; // end class Scheduler

} // end namespace sylar

#endif
