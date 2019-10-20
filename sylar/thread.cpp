#include "thread.h"
#include "log.h"
#include "util.h"

namespace sylar {

//线程局部变量，只属于该线程，在线程开始时分配，结束时释放
static thread_local Thread *t_thread          = nullptr;
static thread_local std::string t_thread_name = "UNKNOWN";

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

Semaphore::Semaphore(uint32_t count) {
    if (sem_init(&m_semaphore, 0, count)) {
        throw ::std::logic_error("sem_init error");
    }
}

Semaphore::~Semaphore() { sem_destroy(&m_semaphore); }

void Semaphore::wait() {
    if (sem_wait(&m_semaphore)) {
        throw std::logic_error("sem_wait error");
    }
}

void Semaphore::notify() {
    if (sem_post(&m_semaphore)) {
        throw std::logic_error("sem_post error");
    }
}

Thread *Thread::GetThis() { return t_thread; }

const std::string &Thread::GetName() { return t_thread_name; }

void Thread::SetName(const std::string &name) {
    if (t_thread) {
        t_thread->m_name = name;
    }
    t_thread_name = name;
}

Thread::Thread(std::function<void()> cb, const std::string &name)
    : m_cb(cb)
    , m_name(name) {
    if (m_name.empty()) {
        m_name = "UNKNOWN";
    }

    int rt = pthread_create(&m_thread, nullptr, &Thread::run, this);
    if (rt) {
        SYLAR_LOG_ERROR(g_logger)
            << "pthread_create thread fail, rt=" << rt << "name=" << name;
        throw std::logic_error("pthread_create error");
    }
}

Thread::~Thread() {
    if (m_thread) {
        pthread_detach(m_thread);
    }
}

void *Thread::run(void *arg) {
    Thread *thread = (Thread *)arg;
    t_thread       = thread;
    t_thread_name  = thread->m_name;
    thread->m_id   = sylar::GetThreadId();

    //修改线程名称，不修改的话会继承父进程的名称，不便于ps查看，线程名称限定长度为16字节
    pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str());

    //设置线程的取消状态和取消类型
    //允许取消
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, nullptr);
    //取消后立即退出，可能存在资源未释放问题，用户线程需要做好应对
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, nullptr);

    std::function<void()> cb;
    cb.swap(thread->m_cb);

    cb();

    return 0;
}

void Thread::join() {
    if (m_thread) {
        int rt = pthread_join(m_thread, nullptr);
        if (rt) {
            SYLAR_LOG_ERROR(g_logger)
                << "pthread_join thread fail, rt=" << rt << "name=" << m_name;
            throw std::logic_error("pthread_join error");
        }
        m_thread = 0;
    }
}

void Thread::cancel() {
    if (m_thread) {
        int rt = pthread_cancel(m_thread);
        if (rt) {
            SYLAR_LOG_ERROR(g_logger)
                << "pthread_cancel thread fail, rt=" << rt << "name=" << m_name;
            throw std::logic_error("pthread_cancel error");
        }
        m_thread = 0;
    }
}

} // namespace sylar