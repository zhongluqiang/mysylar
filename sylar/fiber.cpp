#include "fiber.h"
#include "config.h"
#include "log.h"
#include "macro.h"
#include "scheduler.h"
#include <atomic>

namespace sylar {

static Logger::ptr g_logger = SYLAR_LOG_NAME("system");

namespace {
struct __Initer {
    __Initer() { g_logger->setLevel(LogLevel::INFO); }
};
static __Initer __init;
} // namespace

//全局静态变量，用于生成协程id
static std::atomic<uint64_t> s_fiber_id{0};
//全局静态变量，用于统计当前的协程数
static std::atomic<uint64_t> s_fiber_count{0};

//线程局部变量，当前线程正在运行的协程
static thread_local Fiber *t_fiber = nullptr;
//线程局部变量，当前线程的主协程，切换到这个协程，就相当于切换到了主线程中运行，智能指针形式
static thread_local Fiber::ptr t_threadFiber = nullptr;

//协程栈大小，可通过配置文件获取，默认1M
static ConfigVar<uint32_t>::ptr g_fiber_stacksize =
    ConfigManager::LookUp<uint32_t>("fiber.stacksize", 1024 * 1024,
                                    "fiber stack size");

class MallocStackAllocator {
public:
    static void *Alloc(size_t size) { return malloc(size); }

    static void Dealloc(void *vp, size_t size) { return free(vp); }
};

using StackAllocator = MallocStackAllocator;

uint64_t Fiber::GetFiberId() {
    if (t_fiber) {
        return t_fiber->getId();
    }
    return 0;
}

std::string Fiber::getStateAsString() {
    switch (m_state) {
    case READY:
        return "READY";
    case RUNNING:
        return "RUNNING";
    case TERM:
        return "TERM";
    }
    return "YOU FUCKED UP";
}

//使用默认构造函数的协程对象一定是线程的第一个协程，也就是主协程
Fiber::Fiber() {
    m_state = RUNNING;

    SetThis(this);

    if (getcontext(&m_ctx)) {
        SYLAR_ASSERT2(false, "getcontext");
    }

    ++s_fiber_count;
    m_id = s_fiber_id++; // id从0开始，用完加1

    SYLAR_LOG_DEBUG(g_logger) << "Fiber::Fiber() main id = " << m_id;
}

void Fiber::SetThis(Fiber *f) { t_fiber = f; }

Fiber::ptr Fiber::GetThis() {
    if (t_fiber) {
        return t_fiber->shared_from_this();
    }

    Fiber::ptr main_fiber(new Fiber);
    SYLAR_ASSERT(t_fiber == main_fiber.get());
    t_threadFiber = main_fiber;
    return t_fiber->shared_from_this();
}

//带参数的构造函数用于创建其他协程，需要分配栈
Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool run_in_scheduler)
    : m_id(s_fiber_id++)
    , m_cb(cb)
    , m_runInScheduler(run_in_scheduler) {
    ++s_fiber_count;
    m_stacksize = stacksize ? stacksize : g_fiber_stacksize->getValue();
    m_stack     = StackAllocator::Alloc(m_stacksize);

    if (getcontext(&m_ctx)) {
        SYLAR_ASSERT2(false, "getcontext");
    }

    m_ctx.uc_link          = nullptr;
    m_ctx.uc_stack.ss_sp   = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;

    makecontext(&m_ctx, &Fiber::MainFunc, 0);

    SYLAR_LOG_DEBUG(g_logger) << "Fiber::Fiber() id = " << m_id;
}

Fiber::~Fiber() {
    SYLAR_LOG_DEBUG(g_logger) << "Fiber::~Fiber() id = " << m_id;
    --s_fiber_count;
    if (m_stack) {
        // 有栈，说明是子协程，需要确保子协程一定是结束状态
        SYLAR_ASSERT(m_state == TERM);
        StackAllocator::Dealloc(m_stack, m_stacksize);
        SYLAR_LOG_DEBUG(g_logger) << "dealloc stack, id = " << m_id;
    } else {
        //没有栈，说明是线程的主协程
        SYLAR_ASSERT(!m_cb);              //主协程没有cb
        SYLAR_ASSERT(m_state == RUNNING); //主协程一定是执行状态

        Fiber *cur = t_fiber; //当前协程就是自己
        if (cur == this) {
            SetThis(nullptr);
        }
    }
}

//重置协程函数和状态，复用栈空间，不重新创建栈
void Fiber::reset(std::function<void()> cb) {
    SYLAR_ASSERT(m_stack);
    SYLAR_ASSERT(m_state == TERM);
    m_cb = cb;
    if (getcontext(&m_ctx)) {
        SYLAR_ASSERT2(false, "getcontext");
    }

    m_ctx.uc_link          = nullptr;
    m_ctx.uc_stack.ss_sp   = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;

    makecontext(&m_ctx, &Fiber::MainFunc, 0);
    m_state = READY;
}

//把目标协程和当前正在执行的协程交换，使其进行运行状态
void Fiber::resume() {
    SYLAR_ASSERT(m_state != TERM && m_state != RUNNING);
    SetThis(this);
    m_state = RUNNING;
    if (m_runInScheduler) {
        if (swapcontext(&(Scheduler::GetMainFiber()->m_ctx), &m_ctx)) {
            SYLAR_ASSERT2(false, "swapcontext");
        }
    } else {
        if (swapcontext(&(t_threadFiber->m_ctx), &m_ctx)) {
            SYLAR_ASSERT2(false, "swapcontext");
        }
    }
}

//从当前协程切回线程的主协程
void Fiber::yield() {
    //线程运行完之后还会再yield一次，用于回到主协程，此时状态已为结束状态
    SYLAR_ASSERT(m_state == RUNNING || m_state == TERM);
    SetThis(t_threadFiber.get());
    if (m_state != TERM) {
        m_state = READY;
    }
    if (m_runInScheduler) {
        if (swapcontext(&m_ctx, &(Scheduler::GetMainFiber()->m_ctx))) {
            SYLAR_ASSERT2(false, "swapcontext");
        }
    } else {
        if (swapcontext(&m_ctx, &(t_threadFiber->m_ctx))) {
            SYLAR_ASSERT2(false, "swapcontext");
        }
    }
}

void Fiber::MainFunc() {
    Fiber::ptr cur =
        GetThis(); // GetThis()的shared_from_this()方法让引用计数加1
    SYLAR_ASSERT(cur);

    cur->m_cb();
    cur->m_cb    = nullptr;
    cur->m_state = TERM;

    auto raw_ptr = cur.get(); //手动让t_fiber的引用计数减1
    cur.reset();
    raw_ptr->yield();
    // if (raw_ptr->m_runInScheduler) {
    //     raw_ptr->yieldto(Scheduler::GetMainFiber());
    // } else {
    //     raw_ptr->yield();
    // }
}

} // namespace sylar