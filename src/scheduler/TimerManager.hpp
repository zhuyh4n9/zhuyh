#pragma once

#include <unordered_map>
#include <sys/timerfd.h>
#include <sys/time.h>
#include "../latch/lock.hpp"
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <functional>
#include "../logUtil.hpp"

namespace zhuyh
{
  //基于timerfd
  class Timer final
  {
  public:
    typedef std::shared_ptr<Timer> ptr;
    enum TimerType
      {
	SINGLE, //单次计时
	LOOP //循环计时
      };
    Timer() { memset(&_timer,0,sizeof(_timer));}
    Timer(time_t sec,long msec = 0,long usec = 0,long nsec = 0)
    {
      //LOG_ROOT_ERROR() << "Timer Create";
#define XX(secName1,secName2)	  \
      secName1 += secName2 / 1000; \
      secName2 = secName2 % 1000;
      
      //确保不会溢出
      XX(usec,nsec);
      XX(msec,usec);
      XX(sec,msec);
      
#undef XX
      //LOG_ROOT_INFO() << sec << " "<<_timer.tv_nsec;
      _timer.tv_sec = sec;
      _timer.tv_nsec = nsec + usec*1000L + msec*1000000L;
      create();
    }
    
    ~Timer()
    {
      // LOG_ROOT_INFO() << "timer destroyed _tfd = "<<_tfd;
      if(_tfd != -1)
	{
	  close(_tfd);
	  _tfd = -1;
	}	    
      
    }
    void setLoop()
    {
      _type  = LOOP;
    }
    bool isZero()
    {
      return _timer.tv_sec == 0 && _timer.tv_nsec == 0;
    }
    TimerType getTimerType() const
    {
      return _type;
    }
    
    Timer(const Timer& timer)
    {
      _timer = timer._timer;
      _type = timer._type;
    }
    
    Timer(const timespec& timer)
    {
      _timer = timer;
    }
    
    bool operator<(const Timer& o) const
    {
      if(_timer.tv_sec == o._timer.tv_sec)
	return _timer.tv_nsec < o._timer.tv_nsec;
      return _timer.tv_sec < o._timer.tv_sec;
    }
    
    bool operator>(const Timer& o) const
    {
      if(_timer.tv_sec == o._timer.tv_sec)
	return _timer.tv_nsec > o._timer.tv_nsec;
      return _timer.tv_sec > o._timer.tv_sec;
    }
    
    bool operator==(const Timer& o) const
    {
      return _timer.tv_sec == o._timer.tv_sec
	&&  _timer.tv_nsec == o._timer.tv_nsec;
    }
    int create();
    int start();
    static Timer getCurrentTimer();
    int getTimerFd() const
    {
      return _tfd;
    }
  private:
    int _tfd = -1;    
    struct timespec _timer;
    TimerType _type = SINGLE;
    mutable SpinLock _mx;
  };

  class TimerManager
  {
  public:
    virtual int addTimer(Timer::ptr* timer,std::function<void()> cb,
			  Timer::TimerType type = Timer::SINGLE) = 0;
    virtual int addTimer(Timer::ptr timer,std::function<void()> cb,
			  Timer::TimerType type = Timer::SINGLE) = 0;
    virtual int delTimer(int fd) = 0;
    virtual ~TimerManager() {}
    /*
    //当前时间
    std::list<Timer> listExpiredTimer();
    //指定时间
    std::list<Timer> listExpiredTimer(time_t sec,long msec,long usec = 0);
    std::list<Timer> listExpiredTimer(Timer timer);
    */
  };
  
}
