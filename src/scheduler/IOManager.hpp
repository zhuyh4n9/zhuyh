#pragma once

#include <unistd.h>
#include <sys/timerfd.h>
#include "../concurrent/Thread.hpp"
#include "../concurrent/fiber.hpp"
#include "../latch/lock.hpp"
#include "Scheduler.hpp"
#include <sys/epoll.h>
#include <unordered_map>
#include <memory>
#include <set>
#include <list>
#include <string.h>

namespace zhuyh
{
  //基于timerfd
  struct Timer
  {
    Timer() { memset(&_timer,0,sizeof(_timer));}
    Timer(time_t sec,long msec,long usec = 0,long nsec)
    {
#define XX(secName1,secName2)	  \
      secName1 = secName2 / 1000; \
      secName2 = secName2 % 1000;
      
      XX(usec,nsec);
      XX(msec,usec);
      XX(sec,msec);

#undef XXX
      
      _timer.tv_sec = sec;
      _timer.tv_msec = msec;
      _timer.tv_usec = usec;
      
    }

    ~Timer()
    {
      if(_tfd != -1)
	{
	  close(_tfd);
	  _tfd = -1;
	}	    
    }

    Timer(const Timer& timer) { _timer = timer._timer;}
    Timer(const timespec& timer) { _timer = timer; }
    
    bool operator<(const Timer& o) const
    {
      if(_timer.tv_sec == o._timer.tv_sec)
	return _timer.tv_nsec < o._timer.tv_sec;
      return _timer.tv_sec < o._timer.tv_sec;
    }
    
    bool operator>(const Timer& o) const
    {
      if(_timer.tv_sec == o._timer.tv_sec)
	return _timer.tv_nsec > o._timer.tv_sec;
      return _timer.tv_sec > o._timer.tv_sec;
    }
    
    bool operator==(const Timer& o) const
    {
      return _timer.tv_sec == o._timer.tv_sec
	&&  _timer.tv_nsec == o._timer.tv_sec;
    }
    mutable SpinLock mx;
    int _tfd = -1;    
    struct timespec _timer;
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
  
  class Scheduler;
  struct Task;
  class IOManager : public TimerManager
  {
  public:
    friend class Scheduler;
    enum EventType
      {
	NONE = 0x0,
	READ = EPOLLIN,
	WRITE = EPOLLOUT
      };
    struct EpollEvent 
    {
      Mutex lk;
      typedef std::shared_ptr<EpollEvent> ptr;
      int fd;
      //bool isTimer = false;
      EventType event = NONE;
      std::shared_ptr<Task> rdtask = nullptr;
      std::shared_ptr<Task> wrtask = nullptr;
      EpollEvent(int _fd,EventType _event)
      {
	fd = _fd;
	event = _event;
      }
      EpollEvent() {}
    };
    typedef std::shared_ptr<IOManager> ptr;
    IOManager(const std::string& name = "",Scheduler* scheduler = nullptr);
    ~IOManager();
    int addEvent(int fd,std::shared_ptr<Task> task,EventType type);
    int delEvent(int fd, EventType type);
    //取消应该事件,事件存在则触发事件
    int cancleEvent(int fd,EventType type);
    //取消fd的所有事件并且触发
    int cancleAll(int fd);
    void stop();
    void join()
    {
      _thread->join();
    }
    bool isStopping() const;
    void run();
    void clearAllEvent();
    bool getStopping() const
    {
      return _stopping;
    }
    //设置/获取调度器
    Scheduler* getScheduler();
    void setScheduler(Scheduler* scheduler);
          //触发一个事件
    int triggerEvent(EpollEvent::ptr epEv,EventType type);
    int triggerEvent(EpollEvent* epEv,EventType type);
  private:
    void notify();
    //epoll句柄
    int _epfd = -1;
    Scheduler* _scheduler;
    // fd --> event
    std::unordered_map<int,EpollEvent::ptr> _eventMap;
    mutable RWLock _lk;
    int _notifyFd[2];
    Thread::ptr _thread = nullptr;
    std::string _name;
    std::atomic<bool> _stopping{false};
    std::atomic<int> _holdCount{0};
    EpollEvent::ptr _notifyEvent;
  };
  
}
