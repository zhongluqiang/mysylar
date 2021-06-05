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

    /**
     * @brief 创建调度器，默认将当前线程也作为调度线程
     *
     * @param threads 线程数
     * @param use_caller 是否将当前线程也作为调度线程
     * @param name 名称
     */
    Scheduler(size_t threads = 1, bool use_caller = true,
              const std::string &name = "");
    virtual ~Scheduler();

    const std::string &getName() const { return m_name; }

    static Scheduler *GetThis();
    static Fiber *GetMainFiber();

    /* 添加调度任务 */
    template <class FiberOrCb> void schedule(FiberOrCb fc, int thread = -1) {
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            need_tickle = scheduleNoLock(fc, thread);
        }

        if (need_tickle) {
            tickle(); //唤醒idle协程
        }
    }

    /* 启动协程调度器 */
    void start();
    /* 停止协程调度器，默认等所有协程执行完之后再返回 */
    void stop();

protected:
    /* 通知协程调度器有任务了 */
    virtual void tickle();
    /* 协程调度函数 */
    void run();
    /* 无任务调度时执行idle协程 */
    virtual void idle();
    /* 返回是否可以停止 */
    virtual bool stopping();
    /* 设置当前的协程调度器 */
    void setThis();

private:
    template <class FiberOrCb> bool scheduleNoLock(FiberOrCb fc, int thread) {
        bool need_tickle = m_tasks.empty();
        ScheduleTask task(fc, thread);
        if (task.fiber || task.cb) {
            m_tasks.push_back(task);
        }
        return need_tickle;
    }

private:
    /*调度任务，可以是函数，也可以是Fiber类对象，可指定在哪个线程上调度*/
    struct ScheduleTask {
        Fiber::ptr fiber;
        std::function<void()> cb;
        int thread;

        ScheduleTask(Fiber::ptr f, int thr) {
            fiber  = f;
            thread = thr;
        }
        ScheduleTask(Fiber::ptr *f, int thr) {
            fiber.swap(*f);
            thread = thr;
        }
        ScheduleTask(std::function<void()> f, int thr) {
            cb     = f;
            thread = thr;
        }
        ScheduleTask() { thread = -1; }

        void reset() {
            fiber  = nullptr;
            cb     = nullptr;
            thread = -1;
        }
    };

private:
    std::string m_name;                 //协程调度器名称
    MutexType m_mutex;                  //互斥锁
    std::vector<Thread::ptr> m_threads; //线程池
    std::list<ScheduleTask> m_tasks;    //任务队列
    std::vector<int> m_threadIds;       //线程池的线程ID数组
    size_t m_threadCount = 0; // 工作线程数量，不包含use_caller的主线程
    std::atomic<size_t> m_activeThreadCount = {0}; //活跃线程数
    std::atomic<size_t> m_idleThreadCount   = {0}; // idle线程数

    bool m_useCaller; // 是否use caller
    Fiber::ptr m_rootFiber; // use_caller为true时，调度器所在线程的调度协程
    int m_rootThread = 0; // use_caller为true时，调度器所在线程的id

    bool m_stopping = false; // 是否正在停止
};

} // end namespace sylar

#endif
