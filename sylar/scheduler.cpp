#include "scheduler.h"
#include "macro.h"
#include <errno.h>
#include <fcntl.h> /* Obtain O_* constant definitions */
#include <sys/epoll.h>
#include <unistd.h>

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

Scheduler::Scheduler(const std::string &name)
    : m_name(name) {
    m_epfd = epoll_create1(0);
    SYLAR_ASSERT(m_epfd > 0);

    int rt = pipe(m_tickleFds);
    SYLAR_ASSERT(rt == 0);

    rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
    SYLAR_ASSERT(rt == 0);

    epoll_event ev;
    ev.data.fd = m_tickleFds[0]; //读句柄
    ev.events  = EPOLLIN | EPOLLET;

    rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &ev);
    SYLAR_ASSERT(rt == 0);
}

Scheduler::~Scheduler() {
    //强制停止时可能仍有协程没执行完，这个断言要去掉
    // SYLAR_ASSERT(m_tasks.empty());
}

void Scheduler::start() {
    MutexType::Lock lock(m_mutex);
    SYLAR_ASSERT(m_thread.get() == nullptr);
    m_thread.reset(new Thread(std::bind(&Scheduler::run, this), m_name));
}

void Scheduler::stop(bool force) {
    m_stop = true;
    if (!force) {
        tickle(); //唤醒idle协程
        m_thread->join();
        SYLAR_ASSERT(m_tasks.empty());
    } else {
        m_thread->cancel();
        m_thread->join();
    }
    close(m_epfd);
    close(m_tickleFds[0]);
    close(m_tickleFds[1]);
}

void Scheduler::tickle() {
    //往pipe写入一个数据，引起idle协程的epoll_wait唤醒，实现通知调度功能
    int rt = write(m_tickleFds[1], "T", 1);
    SYLAR_ASSERT(rt == 1);
}

void Scheduler::idle() {
    // bug fixed: idle还没来得及运行，上层就添加了任务时，这里会出错
    // SYLAR_ASSERT(m_tasks.empty());

    m_idle = true;
    /* idle协程阻塞在管道epoll_wait上，唤醒函数是tickle(),时机是添加新任务及停止调度
     */
    epoll_event ev;
    while (true) {
        int rt = epoll_wait(m_epfd, &ev, 1, -1);
        if (rt < 0) {
            if (errno == EINTR) {
                continue;
            } else {
                SYLAR_LOG_ERROR(g_logger)
                    << "epoll_wait error:" << strerror(errno);
                break;
            }
        } else {
            break;
        }
    }

    /* 清空pipe，一次将pipe内的所有内容读完，即有可能多次tickle()被一次消耗掉
     */
    if (ev.data.fd == m_tickleFds[0]) {
        uint8_t dummy[256];
        while (read(m_tickleFds[0], dummy, sizeof(dummy)) > 0)
            ;
    }
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
                cb_fiber->reset(task.cb, task.arg);
            } else {
                cb_fiber.reset(new Fiber(task.cb, task.arg));
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
            /* 当前无任务调度，且停止标志被设置，直接结束调度线程 */
            if (m_stop) {
                break;
            }

            /* 未停止，但无任务可调度，运行idle协程 */
            SYLAR_ASSERT(m_tasks.empty());
            Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
            idle_fiber->resume();
            m_idle = false;

            /* idle协程退出，表示要么添加了新任务，要么停止了调度，这里判断下，如果任
             *务还没调度完，要先将所有任务调度完再退出
             */
            if (!m_tasks.empty()) {
                continue;
            } else {
                break;
            }
        }
    } // end while(true)
}
} // end namespace sylar