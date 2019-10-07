#ifndef MYSYLAR_THREAD_H
#define MYSYLAR_THREAD_H

#include <functional>
#include <memory>
#include <pthread.h>
#include <semaphore.h>
#include <string>
#include <thread>

namespace sylar {

class Semaphore {
public:
    Semaphore(uint32_t count = 0);
    ~Semaphore();

    void wait();
    void notify();

private:
    Semaphore(const Semaphore &)  = delete;
    Semaphore(const Semaphore &&) = delete;
    Semaphore &operator=(const Semaphore &) = delete;

private:
    sem_t m_semaphore;
};

//只读锁
template <class T> struct ReadScopedLockImpl {
public:
    //创建时即加锁
    ReadScopedLockImpl(T &mutex)
        : m_mutex(mutex) {
        lock();
    }

    //销毁时解锁
    ~ReadScopedLockImpl() { unlock(); }

    void lock() {
        if (!m_locked) {
            m_mutex.rdlock();
            m_locked = true;
        }
    }

    void unlock() {
        if (m_locked) {
            m_mutex.unlock();
            m_locked = false;
        }
    }

private:
    T &m_mutex;
    bool m_locked = false;
};

//只写锁
template <class T> struct WriteScopedLockImpl {
public:
    WriteScopedLockImpl(T &mutex)
        : m_mutex(mutex) {
        lock();
    }

    ~WriteScopedLockImpl() { unlock(); }

    void lock() {
        if (!m_locked) {
            m_mutex.wrlock();
            m_locked = true;
        }
    }

    void unlock() {
        if (m_locked) {
            m_mutex.unlock();
            m_locked = false;
        }
    }

private:
    T &m_mutex;
    bool m_locked = false;
};

//读写锁
template <class T> struct ScopedLockImpl {
public:
    ScopedLockImpl(T &mutex)
        : m_mutex(mutex) {
        lock();
    }
    ~ScopedLockImpl() { unlock(); }

    void lock() {
        if (!m_locked) {
            m_mutex.lock();
            m_locked = true;
        }
    }

    void unlock() {
        if (m_locked) {
            m_mutex.unlock();
            m_locked = false;
        }
    }

private:
    T &m_mutex;
    bool m_locked = false;
};

//读写锁实现
class RWMutex {
public:
    typedef ReadScopedLockImpl<RWMutex> ReadLock;
    typedef WriteScopedLockImpl<RWMutex> WriteLock;
    typedef ScopedLockImpl<RWMutex> Lock;

    RWMutex() { pthread_rwlock_init(&m_lock, nullptr); }

    ~RWMutex() { pthread_rwlock_destroy(&m_lock); }

    void rdlock() { pthread_rwlock_rdlock(&m_lock); }

    void wrlock() { pthread_rwlock_wrlock(&m_lock); }

    void unlock() { pthread_rwlock_unlock(&m_lock); }

private:
    pthread_rwlock_t m_lock;
};

class Thread {
public:
    typedef std::shared_ptr<Thread> ptr;
    Thread(std::function<void()> cb, const std::string &name);
    ~Thread();

    pid_t getId() const { return m_id; }
    const std::string getName() const { return m_name; }

    void join();

    static Thread *GetThis();
    static const std::string &GetName();
    static void SetName(const std::string &name);

private:
    //禁止线程类对象拷贝
    Thread(const Thread &)  = delete;
    Thread(const Thread &&) = delete;
    Thread operator=(const Thread &) = delete;

    //线程入口
    static void *run(void *arg);

private:
    pid_t m_id         = -1;
    pthread_t m_thread = 0;
    std::function<void()> m_cb; //线程回调函数
    std::string m_name;
};

} // namespace sylar

#endif