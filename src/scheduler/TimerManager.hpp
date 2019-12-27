/*
        IEvent
          |
	_____
TimerEvent  FdEvent
 */
#pragma once

#include <unordered_map>
#include <sys/timerfd.h>
#include <sys/time.h>

namespace zhuyh
{
  //基于timerfd
  class Timer
  {
  public:
    enum TimerType
      {
	SINGLE, //单次计时
	LOOP //循环计时
      };
    Timer() { memset(&_timer,0,sizeof(_timer));}
    Timer(time_t sec,long msec,long usec = 0,long nsec = 0)
    {
#define XX(secName1,secName2)	  \
      secName1 = secName2 / 1000; \
      secName2 = secName2 % 1000;
      
      //确保不会溢出
      XX(usec,nsec);
      XX(msec,usec);
      XX(sec,msec);

#undef XXX
      
      _timer.tv_sec = sec;
      _timer.tv_nsec = nsec + usec*1000L + msec*1000L;
    }

    ~Timer()
    {
      if(_tfd != -1)
	{
	  close(_tfd);
	  _tfd = -1;
	}	    
    }

    Timer(const Timer& timer) {
      _timer = timer._timer;
    }
    
    Timer(const timespec& timer) {
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
    static Timer getCurrentTimer();
    int getTimerFd() const;
  private:
    mutable SpinLock mx;
    int _tfd = -1;    
    struct timespec _timer;
    TimerType
  };

  class TimerManager
  {
  public:
    virtual void addTimer(Timer&& timer);
    virtual void addTimer(Timer timer);
    virtual void removeTimer(const Timer& timer) ;
  private:
    std::set<Timer> _timerSet;
  protected:
    //当前时间
    std::list<Timer> listExpiredTimer();
    //指定时间
    std::list<Timer> listExpiredTimer(time_t sec,long msec,long usec = 0);
    std::list<Timer> listExpiredTimer(Timer timer);
    mutable SpinLock _mx;
  };
  
}