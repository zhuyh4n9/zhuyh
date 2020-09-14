/*
 * You should Not use the class Fiber to create a coroutine,
 * you are supposed to use the macro "co" to create a coroutine instead
 */

#pragma once

#include "latch/latchUtil.hpp"
#include <memory>
#include <atomic>
#include <ucontext.h>
#include <functional>
#include "Thread.hpp"

extern "C" {
    typedef void* fcontext_t;
    typedef void (*FiberCb)(int);

    intptr_t jump_fcontext(fcontext_t* octx,fcontext_t nctx,
			                intptr_t vp,bool preserve_fpu = false);
    fcontext_t make_fcontext(void* stack,size_t size,FiberCb cb);
};

namespace zhuyh {
using context = fcontext_t;

class Fiber : public UnCopyable,public std::enable_shared_from_this<Fiber> {
public:
    friend class Processer;
    friend class Scheduler;
    friend class Reactor;
    friend class CoSemaphore;
    typedef std::function<void()> CbType;
    typedef std::shared_ptr<Fiber> ptr;
    enum State {
        INIT,
        READY,
        HOLD,
        EXEC,
        TERM,
        EXCEPT
    };
    static std::string getState(int S) {
#define XX(NAME)				\
      if(S == NAME ) return #NAME;
      XX(INIT);
      XX(READY);
      XX(HOLD);
      XX(EXEC);
      XX(TERM);
      XX(EXCEPT);
#undef    XX
      return "UNKNOWN";
    }
    Fiber(CbType cb,size_t size = 0);
    ~Fiber();
    //重置协程回调函数,必须在INIT/TERM状态才能执行
    void reset(CbType cb);
    int getState() const { return _state; }
    void setState(State state) { _state = state; }
public:
    static void setThis(Fiber* fiber);
    static Fiber::ptr getThis() ;

    //协程切换至后台并且设置为READY状态
    static void YieldToReady();
    //当前协程切换至后台并且设置为SWITCH状态
    static void YieldToHold();
    //执行函数
    static void run(int);

    static uint32_t getFid();
private:
    Fiber();
    //切换到当前协程,不暴露给用户防止用户将主协程切换到前台导致系统崩溃
    void swapIn();
    //切换到后台,不设置状态因此不暴露给用户
    void swapOut();
    //切换到某一个协程
    void swapTo(Fiber::ptr fiber);
    //设置当前main_fiber(Processer使用)
    static Fiber::ptr getMain()  { return _main_fiber(); }
    static void setMain(Fiber* fiber) { _main_fiber().reset(fiber); }
private:
    CbType _cb;
    uint32_t _fid = 0;
    context _ctx;
    //协程状态
    std::atomic<int> _state {INIT};
public:
    //协程栈
    char* _stack = nullptr;
private:
    bool isMain = false;
    size_t _stackSize = 0;
    size_t _pagesProtect = 0;

    static Fiber::ptr& _main_fiber();
    static Fiber*& _this_fiber();
};

}
