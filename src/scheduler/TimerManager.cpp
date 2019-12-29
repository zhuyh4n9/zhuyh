#include "TimerManager.hpp"
#include "../logUtil.hpp"
#include "../macro.hpp"

namespace zhuyh
{
  static Logger::ptr sys_log = GET_LOGGER("system");
  Timer Timer::getCurrentTimer()
  {
    struct timespec timer;
    clock_gettime(CLOCK_MONOTONIC,&timer);
    return Timer(timer);
  }
  int Timer::create()
  {
    if(_tfd == -1)
      {
	//不确定timer_create是否线程安全,因此加一个自旋锁
	//LockGuard lg(_mx);
	_tfd = timerfd_create(CLOCK_MONOTONIC,TFD_NONBLOCK);
	if(_tfd < 0 ) throw std::logic_error("failed to create timerfd");
      }
    return _tfd;
  }
  //开启计时
  int Timer::start()
  {
    ASSERT(_tfd!=-1);
    //LOG_INFO(sys_log)<<"starting _tfd = " << _tfd;
    struct timespec tm  = getCurrentTimer()._timer;
    struct itimerspec new_timer;
    bzero(&new_timer,sizeof(new_timer));
    new_timer.it_value.tv_sec = tm.tv_sec + _timer.tv_sec;
    new_timer.it_value.tv_nsec = tm.tv_nsec + _timer.tv_nsec;    
    if(timerfd_settime(_tfd,TFD_TIMER_ABSTIME,&new_timer,nullptr) < 0)
      {
	LOG_ERROR(sys_log) << "timerfd_create : "<<strerror(errno);
	throw std::logic_error("failed to create timerfd");
      }
    //LOG_INFO(sys_log)<<"started : _tfd = "<<_tfd;
    return _tfd;
  }
  
}
