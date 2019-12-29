/*
 * You should Not use the class Fiber to create a coroutine,
 * you are supposed to use the macro "co" to create a coroutine instead
 */

#pragma once

#include "../latch/latchUtil.hpp"
#include <memory>
#include <atomic>
#include <ucontext.h>
#include <functional>
#include "Thread.hpp"

extern "C"
{
  typedef void* fcontext_t;
  typedef void (*FiberCb)(int);
  
  intptr_t jump_fcontext(fcontext_t* octx,fcontext_t nctx,
			 intptr_t vp,bool preserve_fpu = false);
  fcontext_t make_fcontext(void* stack,size_t size,FiberCb cb);
};

namespace zhuyh
{
  //TODO:将来协程实现为boost.context
    using context = fcontext_t;
  //using context = ucontext_t;
  class Fiber : public UnCopyable,public std::enable_shared_from_this<Fiber>
  {
  public:
    //Processer会使用到swapIn方法,因此为该类的友元
    friend class Processer;
    friend class Scheduler;
    friend class IOManager;
    typedef std::function<void()> CbType;
    typedef std::shared_ptr<Fiber> ptr;
    enum State
      {
	INIT,
	READY,
	HOLD,
	SWITCHING, //用于HOLD过渡,防止jump_fcontext被执行多次
	EXEC,
	TERM,
	EXCEPT
      };
    static std::string getState(int S)
    {
#define XX(NAME)				\
      if(S == NAME ) return #NAME;
      XX(INIT);
      XX(READY);
      XX(HOLD);
      XX(EXEC);
      XX(TERM);
      XX(EXCEPT);
      XX(SWITCHING);
#undef    XX
      return "UNKNOWN";
    }
    //size为0则使用默认大小
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
    //协程切换至后台并且设置为HOLD状态
    static void YieldToHold();
    static void YieldToSwitch();
    //获取总协程数
    static uint64_t totalFibers();
    //执行函数
    static void run(int);

    static uint32_t getFid();

    static uint32_t getTotalActive();

    static uint32_t getLocalActive();
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
    //协程ID
    uint32_t _fid = 0;
    //上下文
    context _ctx;
    //协程状态
    std::atomic<int> _state {INIT};
  public:
    //协程栈
    char* _stack = nullptr;
  private:
    //是否是主协程
    bool isMain = false;
    //栈大小
    size_t _stackSize = 0;
    //保护的页的数量
    size_t _pagesProtect = 0;
    //静态"成员变量"
    static uint32_t& _fiber_local_not_term()
    {
      static thread_local uint32_t __fiber_local_not_term{0};
      return __fiber_local_not_term;
    }
    static Fiber*& _this_fiber()
    {
      static thread_local Fiber* __this_fiber = nullptr;
      return __this_fiber;
    }
    static Fiber::ptr& _main_fiber()
    {
      static thread_local Fiber::ptr _threadFiber = nullptr;
      return _threadFiber;
    }
  };
  
}
