#pragma once

#include <set>
#include <vector>
#include "../latch/lock.hpp"
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <functional>
#include <list>
/*
 *时间均采用开机时间,不需要担心时间被推迟
 */
namespace zhuyh
{
  class Task;
  class TimerManager;
  class Timer : public std::enable_shared_from_this<Timer>
  {
  public:
    typedef std::function<void()> CbType;
    typedef std::shared_ptr<Timer> ptr;
    friend class TimerManager;
    enum TimerType
      {
	SINGLE, //单次计时
	LOOP //循环计时
      };
    
    Timer(time_t sec = 0,time_t msec = 0,time_t usec = 0,time_t nsec = 0);
    
    Timer(uint64_t* msec)
    {
      _cancled = false;
      _nxtExpireTime = *msec;
    }
    void setManager(TimerManager* manager);
    ~Timer()
    {
      // LOG_ROOT_INFO() << "timer destroyed _tfd = "<<_tfd;
    }
    static uint64_t getCurrentTime();
    TimerType getTimerType() const
    {
      return _type;
    }
    void start();
    void setLoop()
    {
      if(_start) return;
      _type = LOOP;
    }
    bool cancle();
    bool isCancled() const
    {
      return _cancled;
    }
    //nullptr表示使用当前协程,否则表示回调函数
    void setTask(CbType cb = nullptr);
    std::shared_ptr<Task> getTask()
    {
      return _task;
    }
  private:
    void setNextExpireTime()
    {
      if(_type == SINGLE) return;
      _nxtExpireTime += _interval;
    }
  private:
    std::shared_ptr<Task> _task = nullptr;
    bool _start = false;
    bool _cancled = false;
    TimerType _type = SINGLE;
    uint64_t _interval = 0;
    uint64_t _nxtExpireTime = 0;
    TimerManager* _manager = nullptr;
  };

  class TimerManager 
  {
  public:
    typedef std::shared_ptr<TimerManager> ptr;
    friend class Timer;
    int addTimer(Timer::ptr* timer,
		 std::function<void()> cb = nullptr,
		 Timer::TimerType type = Timer::SINGLE);
    int addTimer(Timer::ptr timer,
		 std::function<void()> cb = nullptr,
		 Timer::TimerType type = Timer::SINGLE);
    virtual ~TimerManager() {}
    //获取下一次超时时间
    uint64_t getNextExpireTime();
    //获取距离下一次超时的间隔时间
    uint64_t getNextExpireInterval();
    virtual void notify() = 0;
    //列出所有超时的Timer
    std::list<std::shared_ptr<Task> > getExpiredTasks();
  public:
    class Comparator
    {
    public:
      bool operator() (const Timer::ptr& o1,const Timer::ptr& o2) const;
    };
  private:
    std::set<Timer::ptr,Comparator> _timers;
    mutable Mutex _mx;
  };
  
}
