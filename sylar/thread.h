#ifndef MYSYLAR_THREAD_H
#define MYSYLAR_THREAD_H

#include <atomic>
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

//读写锁操作实现
class RWMutex {
public:
    typedef ReadScopedLockImpl<RWMutex> ReadLock;
    typedef WriteScopedLockImpl<RWMutex> WriteLock;

    RWMutex() { pthread_rwlock_init(&m_lock, nullptr); }

    ~RWMutex() { pthread_rwlock_destroy(&m_lock); }

    void rdlock() { pthread_rwlock_rdlock(&m_lock); }

    void wrlock() { pthread_rwlock_wrlock(&m_lock); }

    void unlock() { pthread_rwlock_unlock(&m_lock); }

private:
    pthread_rwlock_t m_lock;
};

//互斥锁，不区分读写
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

//互斥锁操作实现
class Mutex {
public:
    typedef ScopedLockImpl<Mutex> Lock;

    Mutex() { pthread_mutex_init(&m_mutex, nullptr); }

    ~Mutex() { pthread_mutex_destroy(&m_mutex); }

    void lock() { pthread_mutex_lock(&m_mutex); }

    void unlock() { pthread_mutex_unlock(&m_mutex); }

private:
    pthread_mutex_t m_mutex;
};

//自旋锁操作实现
class Spinlock {
public:
    typedef ScopedLockImpl<Spinlock> Lock;
    Spinlock() { pthread_spin_init(&m_mutex, 0); }

    ~Spinlock() { pthread_spin_destroy(&m_mutex); }

    void lock() { pthread_spin_lock(&m_mutex); }

    void unlock() { pthread_spin_unlock(&m_mutex); }

private:
    pthread_spinlock_t m_mutex;
};

//使用atomic_flag在用户空间实现的自旋锁
class AtomicSpinlock {
public:
    typedef ScopedLockImpl<AtomicSpinlock> Lock;

    AtomicSpinlock() { m_mutex.clear(); }

    ~AtomicSpinlock() {}

    void lock() {
        while (std::atomic_flag_test_and_set_explicit(
            &m_mutex, std::memory_order_acquire))
            ;
    }

    void unlock() {
        std::atomic_flag_clear_explicit(&m_mutex, std::memory_order_release);
    }

private:
    volatile std::atomic_flag m_mutex;
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