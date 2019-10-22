#ifndef MYSYLAR_SCHEDULER_H
#define MYSYLAR_SCHEDULER_H

#include "fiber.h"
#include "log.h"
#include "thread.h"
#include <functional>
#include <list>
#include <memory>
#include <string>

namespace sylar {

class Scheduler {
public:
    typedef std::shared_ptr<Scheduler> ptr;
    typedef Mutex MutexType;

    Scheduler(const std::string &name = "");
    virtual ~Scheduler();

    /* 添加调度任务 */
    template <class FiberOrCb> void schedule(FiberOrCb fc) {
        bool need_tickle = false;
        MutexType::Lock lock(m_mutex);
        ScheduleTask task(fc);

        if (task.fiber || task.cb) {

            /* 如果当前是idle状态，需要唤醒idle协程，使其立即退出，进行重新调度*/
            if (m_idle) {
                need_tickle = true;
            }
            m_tasks.push_back(task);
        }

        if (need_tickle) {
            tickle(); //唤醒idle协程
        }
    }

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
    /*调度任务，可以是函数，也可以是Fiber类对象*/
    struct ScheduleTask {
        Fiber::ptr fiber;
        std::function<void()> cb;

        ScheduleTask(Fiber::ptr f) { fiber = f; }
        ScheduleTask(Fiber::ptr *f) { fiber.swap(*f); }
        ScheduleTask(std::function<void()> f) { cb = f; }
        ScheduleTask() {}

        void reset() {
            fiber = nullptr;
            cb    = nullptr;
        }
    };

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
};

} // end namespace sylar

#endif
