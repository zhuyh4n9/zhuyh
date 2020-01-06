#include "Hook.hpp"
#include <dlfcn.h>
#include "../scheduler/Scheduler.hpp"
#include "../macro.hpp"
#include "../logUtil.hpp"
#include "../scheduler/TimerManager.hpp"

namespace zhuyh
{
  extern "C"
  {
    sleep_func sleep_f;
    usleep_func usleep_f;
    nanosleep_func nanosleep_f;
    fcntl_func fcntl_f;
  }
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
  XX(usleep);					\
  XX(nanosleep);				\
  XX(fcntl);
  
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
    {
      //LOG_ROOT_ERROR() << "USE ORIGIN";
      return sleep_f(seconds);
    }
  auto scheduler = zhuyh::Scheduler::getThis();
  ASSERT(scheduler != nullptr);
  scheduler->addTimer(zhuyh::Timer::ptr(new zhuyh::Timer((time_t)seconds)));
  return 0;
}

int usleep(useconds_t usec)
{
  if(zhuyh::Hook::isHookEnable() == false)
    {
      //LOG_ROOT_ERROR() << "USE ORIGIN";
      return usleep_f(usec);
    }
  auto scheduler = zhuyh::Scheduler::getThis();
  ASSERT(scheduler != nullptr);
  usec /= 1000;
  scheduler->addTimer(zhuyh::Timer::ptr(new zhuyh::Timer(0,(time_t)usec)));
  return 0;
}

int nanosleep(const struct timespec *req, struct timespec *rem)
{
  if(zhuyh::Hook::isHookEnable() == false)
    return nanosleep_f(req,rem);
  auto scheduler = zhuyh::Scheduler::getThis();
  ASSERT(scheduler != nullptr);
  time_t sec = req->tv_sec;
  time_t nsec = req->tv_nsec;
  if(nsec <0 || nsec > 1000000000 || sec < 0 || sec > 1000000000)
    {
      errno = EINVAL;
      return -1;
    }
  nsec /=1000000;
  scheduler->addTimer(zhuyh::Timer::ptr(new zhuyh::Timer(sec,(time_t)nsec)));
  return 0;
}