#ifndef MYSYLAR_FIBER_H
#define MYSYLAR_FIBER_H

#include "thread.h"
#include <functional>
#include <memory>
#include <ucontext.h>

namespace sylar {

class Fiber : public std::enable_shared_from_this<Fiber> {
public:
    typedef std::shared_ptr<Fiber> ptr;

    enum State {
        FIBER_INIT,      //刚创建协程时的状态
        FIBER_RUNNING,   //运行状态
        FIBER_PENDING,   //阻塞状态
        FIBER_TERMINATED //结束状态
    };

private:
    //默认构造函数只用于创建线程的第一个协程
    Fiber();

public:
    //公共构造函数用于创建用户协程，需要分配栈
    Fiber(std::function<void(void *)> cb, void *arg = nullptr,
          size_t stacksize = 0);
    ~Fiber();

    //重置协程状态和入口函数，复用栈空间，不重新创建栈
    void reset(std::function<void(void *)> cb, void *arg = nullptr);

    //把目标协程和当前正在执行的协程进行交换，使其进入运行状态
    void resume();

    //协程让出执行权，并且设置状态为FIBER_PENDING
    void yield();

    //返回某个协程的协程号
    uint64_t getId() const { return m_id; }
    State getState() const { return m_state; }

public:
    //设置当前正在运行的协程，即设置线程局部变量t_fiber的值
    static void SetThis(Fiber *f);

    //返回当前线程正在执行的协程，如果当前线程还未创建协程，则创建线程的第一个协程，
    //且该协程为当前线程的主协程，其他协程都通过这个协程来调度，也就是说，其他协程
    //结束时,都要切回到主协程，由主协程重新选择新的协程进行resume
    static Fiber::ptr GetThis();

    //总协程数
    static uint64_t TotalFibers();

    //用于执行协程的函数
    static void MainFunc(void *arg);

    //当前运行的协程的的协程号
    static uint64_t GetFiberId();

private:
    uint64_t m_id        = 0;          //协程号
    uint32_t m_stacksize = 0;          //协程栈大小
    State m_state        = FIBER_INIT; //协程状态

    ucontext_t m_ctx;        //协程上下文
    void *m_stack = nullptr; //协程栈

    std::function<void(void *)> m_cb; //协程入口
    void *m_arg;                      //协程参数
};

} // namespace sylar

#endif