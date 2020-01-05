#include "Hook.hpp"
#include <dlfcn.h>
#include "../scheduler/Scheduler.hpp"
#include "../macro.hpp"
#include "../logUtil.hpp"
#include "../scheduler/TimerManager.hpp"
extern "C"
{
  sleep_func sleep_f;
  usleep_func usleep_f;
}
namespace zhuyh
{
  thread_local bool __hook_state__ = false;
  void Hook::setHookState(State state)
  {
    if(state == OFF)
      __hook_state__ = false;
    else
      __hook_state__ = true;
  }
  
  bool Hook::isHookEnable()
  {
    return __hook_state__;
  }
  
#define XX(name)					\
  name ## _f = (name ## _func) dlsym(RTLD_NEXT,#name);
  
#define HOOK_FUNC()				\
  XX(sleep);					\
  XX(usleep);					

  struct HookInit
  {
    HookInit()
    {
      static bool initTag = false;
      if(initTag == false)
	{
	  HOOK_FUNC();
	  initTag = true;
	}
      
    }
  };
#undef XX
#undef HOOK_FUNC

  static HookInit __hook_initer__;
}

unsigned int sleep(unsigned int seconds)
{
  if(zhuyh::Hook::isHookEnable() == false)
    return sleep_f(seconds);
  auto scheduler = zhuyh::Scheduler::getThis();
  ASSERT(scheduler != nullptr);
  scheduler->addTimer(zhuyh::Timer::ptr(new zhuyh::Timer((time_t)seconds)));
  return 0;
}

int usleep(useconds_t usec)
{
  if(zhuyh::Hook::isHookEnable() == false)
    return usleep_f(usec);
  auto scheduler = zhuyh::Scheduler::getThis();
  ASSERT(scheduler != nullptr);
  usec /= 1000;
  scheduler->addTimer(zhuyh::Timer::ptr(new zhuyh::Timer(0,(time_t)usec)));
  return 0;
}

