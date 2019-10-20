#include "scheduler.h"
#include "macro.h"

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

Scheduler::Scheduler(const std::string &name)
    : m_name(name) {}

Scheduler::~Scheduler() { SYLAR_ASSERT(m_tasks.empty()); }

void Scheduler::start() {
    MutexType::Lock lock(m_mutex);
    SYLAR_ASSERT(m_thread.get() == nullptr);
    m_thread.reset(new Thread(std::bind(&Scheduler::run, this), m_name));
}

void Scheduler::stop(bool force) {
    if (!force) {
        tickle(); //唤醒idle协程
        m_thread->join();
        SYLAR_ASSERT(m_tasks.empty());
    } else {
        m_thread->cancel();
        m_thread->join();
    }
}

void Scheduler::tickle() {
    // SYLAR_LOG_INFO(g_logger) << "ticlke";
    m_semIdle.notify();
}

void Scheduler::idle() {
    SYLAR_ASSERT(m_tasks.empty());
    m_idle = true;
    /* idle协程阻塞在信号量wait上，唤醒函数是tickle(),时机是添加新任务及停止调度
     */
    m_semIdle.wait();
}

void Scheduler::run() {
    Fiber::GetThis();
    ScheduleTask task;
    Fiber::ptr cb_fiber;

    while (true) {
        task.reset();

        /* 从任务队列中获取一个要执行的任务 */
        {
            MutexType::Lock lock(m_mutex);
            auto it = m_tasks.begin();
            while (it != m_tasks.end()) {
                SYLAR_ASSERT(it->fiber || it->cb);
                task = *it;
                m_tasks.erase(it);
                ++it;
                break;
            }
        }

        /* 调度任务，没有任务时执行idle协程 */
        if (task.fiber && task.fiber->getState() != Fiber::FIBER_TERMINATED) {
            task.fiber->resume();
            if (task.fiber->getState() != Fiber::FIBER_TERMINATED) {
                schedule(task.fiber);
            } else {
                task.fiber.reset();
            }
        } else if (task.cb) {
            if (cb_fiber) {
                /* 直接复用原来的fiber */
                cb_fiber->reset(task.cb);
            } else {
                cb_fiber.reset(new Fiber(task.cb));
            }
            cb_fiber->resume();
            if (cb_fiber->getState() != Fiber::FIBER_TERMINATED) {
                schedule(cb_fiber);
                cb_fiber.reset();
            } else {
                /* 协程结束不回收协程对象，便于下次复用 */
                cb_fiber->reset(nullptr);
            }
        } else {
            /* 无任务可调度，运行idle协程 */
            SYLAR_ASSERT(m_tasks.empty());
            Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
            idle_fiber->resume();
            m_idle = false;

            /* idle协程只有添加了新任务或是调度器停止时才会被唤醒，如果没有调度任务，
             * 则一定是调用了stop()方法，是时候结束整个协程调度了*/
            if (!m_tasks.empty()) {
                continue;
            } else {
                break;
            }
        }
    } // end while(true)
}
} // end namespace sylar