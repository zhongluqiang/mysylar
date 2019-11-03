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

    resizeFdContext(32);
}

Scheduler::~Scheduler() {
    //强制停止时可能仍有协程没执行完，这个断言要去掉
    // SYLAR_ASSERT(m_tasks.empty());
    for (size_t i = 0; i < m_fdContexts.size(); ++i) {
        if (m_fdContexts[i]) {
            delete m_fdContexts[i];
        }
    }
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
    SYLAR_LOG_DEBUG(g_logger) << "idle begin";
    m_idle = true;

    /* idle协程阻塞在管道epoll_wait上，唤醒函数是tickle(),时机是添加新任务及停止调度
     */
    epoll_event *events = new epoll_event[64]();
    std::shared_ptr<epoll_event> events_deallocator(
        events, [](epoll_event *ptr) { delete[] ptr; });

    while (true) {
        int rt = epoll_wait(m_epfd, events, 64, -1);
        if (rt < 0) {
            if (errno == EINTR) {
                continue;
            } else {
                SYLAR_LOG_ERROR(g_logger)
                    << "epoll_wait error:" << strerror(errno);
                break;
            }
        }

        SYLAR_LOG_DEBUG(g_logger) << "epoll_wait return,rt=" << rt;

        for (int i = 0; i < rt; ++i) {
            /* pipe有数据，说明有任务需要调度 */
            if (events[i].data.fd == m_tickleFds[0]) {
                /* 清空pipe，一次将pipe内的所有内容读完，即有可能多次tickle()被一次
                 * 消耗掉
                 */
                uint8_t dummy[256];
                while (read(m_tickleFds[0], dummy, sizeof(dummy)) > 0)
                    ;
                continue;
            }
            FdContext *fd_ctx    = (FdContext *)events[i].data.ptr;
            fd_ctx->event.events = events[i].events;
            SYLAR_LOG_DEBUG(g_logger)
                << "IO fibler fd=" << fd_ctx->fd << " has event";

            if (fd_ctx->task.cb) {
                schedule(fd_ctx->task.cb,
                         (void *)fd_ctx); //这里多tickle了一下，可以优化掉
            } else if (fd_ctx->task.fiber) {
                schedule(fd_ctx->task.fiber);
            } else {
                SYLAR_LOG_ERROR(g_logger) << "invalid task";
            }
        }
        // TODO:待优化，这里不需要break，因为idle也是一个协程，直接在这里yield就可以了，能
        //节省掉创新新idle协程的开销
        SYLAR_LOG_DEBUG(g_logger) << "break idle";
        break;
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

            // /*
            // idle协程退出，表示要么添加了新任务，要么停止了调度，这里判断下，如果任
            //  *务还没调度完，要先将所有任务调度完再退出
            //  */
            // if (!m_tasks.empty()) {
            //     continue;
            // } else {
            //     SYLAR_LOG_INFO(g_logger) << "Schedules thread exit";
            //     break;
            // }
        }
    } // end while(true)
} // end Scheduler::run()

void Scheduler::resizeFdContext(size_t size) {
    m_fdContexts.resize(size);

    for (size_t i = 0; i < m_fdContexts.size(); ++i) {
        if (!m_fdContexts[i]) {
            m_fdContexts[i] = new FdContext();
        }
    }
}

int Scheduler::io_schedule(int fd, int op, uint32_t events, ScheduleTask task) {
    if (fd < 0) {
        SYLAR_LOG_ERROR(g_logger) << "Invalid fd:" << fd;
        return -1;
    }

    int rt;
    MutexType::Lock lock(m_mutex);
    FdContext *fd_ctx = nullptr;

    if ((int)m_fdContexts.size() > fd) {
        fd_ctx = m_fdContexts[fd];
    } else {
        resizeFdContext(fd * 1.5);
        fd_ctx = m_fdContexts[fd];
    }

    // op及fd_ctx合法性检测
    if (op == EPOLL_CTL_ADD) {
        if (fd_ctx->fd >= 0) {
            //重复添加，返回错误
            SYLAR_LOG_ERROR(g_logger)
                << "error,fd " << fd << " has been added!";
            return -1;
        }
    } else if (op == EPOLL_CTL_MOD) {
        if (fd_ctx->fd < 0) {
            //待修改的fd未添加，返回错误
            SYLAR_LOG_ERROR(g_logger)
                << "error,fd " << fd << " has not been added!";
            return -1;
        }
    } else if (op == EPOLL_CTL_DEL) {
        if (fd_ctx->fd != fd) {
            //待删除的fd未添加，返回错误
            SYLAR_LOG_ERROR(g_logger)
                << "error,fd " << fd << " has not been added!";
            return -1;
        }
    } else {
        SYLAR_LOG_ERROR(g_logger) << "invalid op: " << op;
        return -1;
    }

    //设置fd为非阻塞模式
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        SYLAR_LOG_ERROR(g_logger) << "fcntl(" << fd << ", F_GETFL)"
                                  << "failed:" << strerror(errno);
        return -1;
    }
    flags |= O_NONBLOCK;
    rt = fcntl(fd, F_SETFL, flags);
    if (rt < 0) {
        SYLAR_LOG_ERROR(g_logger) << "fcntl(" << fd << ", F_SETFL)"
                                  << "failed:" << strerror(errno);
        return -1;
    }

    fd_ctx->fd             = fd;
    fd_ctx->task           = task;
    fd_ctx->scheduler      = this;
    fd_ctx->event.events   = 0;      //这里保存已触发的事件集合
    fd_ctx->event.data.u32 = events; //这里用来保存原始的事件集合

    epoll_event ev;
    ev.events   = events;
    ev.data.ptr = fd_ctx;

    rt = epoll_ctl(m_epfd, op, fd, &ev);
    if (rt) {
        SYLAR_LOG_ERROR(g_logger)
            << "epoll_ctl(" << m_epfd << ", " << op << "," << fd << ","
            << ev.events << "):" << rt << " (" << errno << ") ("
            << strerror(errno) << ")";
        return -1;
    }

    if (op == EPOLL_CTL_DEL) {
        fd_ctx->reset();
    }

    return 0;
} // end Scheduler::io_schedule

int Scheduler::timer_schedule(Timer::ptr timer, int op) {
    if (!timer) {
        SYLAR_LOG_ERROR(g_logger) << "invalid argument:" << timer;
        return -1;
    }

    Timer::TimerContext *pContext = new Timer::TimerContext;
    SYLAR_ASSERT(pContext != nullptr);
    pContext->fd   = timer->getcontext().fd;
    pContext->task = timer->getcontext().task;
    Fiber::ptr fiber(new Fiber(&Timer::timer_proc, pContext));

    return io_schedule(pContext->fd, op, EPOLLIN | EPOLLET, fiber);
} // end Scheduler::timer_schedule

} // end namespace sylar