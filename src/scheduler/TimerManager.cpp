#include "TimerManager.hpp"
#include "../logUtil.hpp"
#include "../macro.hpp"

namespace zhuyh
{
  static Logger::ptr sys_log = GET_LOGGER("system");
  Timer Timer::getCurrentTimer()
  {
    struct timespec timer;
    clock_gettime(CLOCK_REALTIME,&timer);
    return Timer(timer);
  }
  int Timer::create()
  {
    if(_tfd == -1)
      {
	//不确定timer_create是否线程安全,因此加一个自旋锁
	//LockGuard lg(_mx);
	_tfd = timerfd_create(CLOCK_REALTIME,TFD_NONBLOCK);
	//ASSERT2(_tfd >= 0,strerror(errno));
	if(_tfd < 0 ) throw std::logic_error("failed to create timerfd");
      }
    return _tfd;
  }
  //开启计时
  int Timer::start()
  {
    ASSERT(_tfd!=-1);
    if(_tfd == -1)
      {
	
      }
    //LOG_INFO(sys_log)<<"starting _tfd = " << _tfd;
    struct timespec tm  = getCurrentTimer()._timer;
    struct itimerspec new_timer;
    bzero(&new_timer,sizeof(new_timer));
    //防止纳秒超过10亿
    new_timer.it_value.tv_sec = tm.tv_sec + _timer.tv_sec + (tm.tv_nsec + _timer.tv_nsec)/1000000000L;
    new_timer.it_value.tv_nsec = (tm.tv_nsec + _timer.tv_nsec)%1000000000L;
    if(timerfd_settime(_tfd,TFD_TIMER_ABSTIME,&new_timer,nullptr) < 0)
      {
	LOG_ERROR(sys_log) << "timerfd_settime : "<<strerror(errno)
			   <<" \n_tfd = "<<_tfd <<" new_timer.tv_sec = "<<new_timer.it_value.tv_sec
			   <<" new_timer.tv_nsec = "<<new_timer.it_value.tv_nsec;
	throw std::logic_error("failed to setting timerfd");
      }
    //LOG_INFO(sys_log)<<"started : _tfd = "<<_tfd;
    return _tfd;
  }
  
}
