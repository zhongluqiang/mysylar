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
        READY,   // 就绪态，刚创建或者yield之后的状态
        RUNNING, // 运行态，resume之后的状态
        TERM // 结束态，协程的回调函数执行完之后为TERM状态
    };

private:
    //默认构造函数只用于创建线程的第一个协程
    Fiber();

public:
    //公共构造函数用于创建用户协程，需要分配栈
    Fiber(std::function<void()> cb, size_t stacksize = 0,
          bool run_in_scheduler = true);
    ~Fiber();

    //重置协程状态和入口函数，复用栈空间，不重新创建栈
    void reset(std::function<void()> cb);

    //把目标协程和当前正在执行的协程进行交换，使其进入运行状态
    void resume();

    //协程让出执行权，并且设置状态为READY
    void yield();

    /// 和指定的协程进行swap，目标协程会被执行，当前协程挂起
    // void yieldto(Fiber *f);

    //返回某个协程的协程号和协程状态
    uint64_t getId() const { return m_id; }
    State getState() const { return m_state; }
    std::string getStateAsString();

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
    static void MainFunc();

    //当前运行的协程的的协程号
    static uint64_t GetFiberId();

private:
    uint64_t m_id        = 0;     //协程号
    uint32_t m_stacksize = 0;     //协程栈大小
    State m_state        = READY; //协程状态

    ucontext_t m_ctx;        //协程上下文
    void *m_stack = nullptr; //协程栈

    std::function<void()> m_cb; //协程入口

    bool m_runInScheduler; // 该协程是否参与调度器调度
};

} // namespace sylar

#endif