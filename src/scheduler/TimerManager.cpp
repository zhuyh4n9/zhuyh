#include "TimerManager.hpp"
#include "../macro.hpp"
#include "../logUtil.hpp"
#include "Task.hpp"
#include <list>
#include <vector>
#include <algorithm>
namespace zhuyh
{
  static Logger::ptr sys_log = GET_LOGGER("system");
  Timer::Timer(time_t sec,time_t msec ,time_t usec ,time_t nsec)
  {
    _cancled = false;
    ASSERT(sec >= 0 && msec >= 0 && usec >= 0 && nsec >= 0);
    usec += nsec/1000;
    msec += usec/1000;
    _interval = ((uint64_t)msec)+((uint64_t)sec)*1000ull;
    //开机时间
    _nxtExpireTime = _interval + getCurrentTime();
    //LOG_ROOT_INFO() << "nxt : "<<_nxtExpireTime;
    _type = SINGLE;
  }
  void Timer::start()
  {
    ASSERT(_start == false);
    //必须设置完任务之后定时器才可以开始
    ASSERT(_task != nullptr);
    _start = true;
  }
  uint64_t Timer::getCurrentTime()
  {
    struct timespec timer;
    clock_gettime(CLOCK_MONOTONIC,&timer);
    return ( ((uint64_t)timer.tv_nsec)/1000000ull) + ((uint64_t)timer.tv_sec)*1000ull;
  }
  
  void Timer::setManager(TimerManager* manager)
  {
    _manager = manager;
  }
  
  bool Timer::cancle()
  {
    //ASSERT(0);
    
    ASSERT(_manager != nullptr);
    WRLockGuard(_manager->_mx);
    if(_cancled) return false;
    _cancled = true;
    auto it = _manager->_timers.find(shared_from_this());
    if(it != _manager->_timers.end())
      _manager->_timers.erase(it);
    return true;
  }
  void Timer::setTask(CbType cb)
  {
    ASSERT(_cancled == false);
    ASSERT(_start == false);
    if(cb == nullptr)
      {
	_task.reset(new Task(Fiber::getThis()));
      }
    else
      {
	_task.reset(new Task(cb));
      }
  }
  bool TimerManager::Comparator::operator() (const Timer::ptr& o1,const Timer::ptr& o2) const
  {
    ASSERT(o1 != nullptr && o2 != nullptr);
    if(*o1 != *o2)
      return *o1 < *o2;
    return o1.get() < o2.get();
  }
  std::list<Task::ptr> TimerManager::getExpiredTasks()
  {
    std::list<Task::ptr> res;
    std::list<Timer::ptr> tms;
    uint64_t minExpireTime = (uint64_t)-1;
    WRLockGuard lg(_mx);
    //LOG_ROOT_INFO() << "total : "<<_timers.size();
    if(_timers.empty() ) return res;
    auto t = Timer::ptr(new Timer());
    std::set<Timer::ptr>::iterator pos = _timers.upper_bound(t);
    // while(pos!= _timers.end() && ( (*pos)->_nxtExpireTime == t->_nxtExpireTime) )
    //   pos++;
    for(auto it = _timers.begin();it != pos; it++)
      {
	ASSERT((*it)->isCancled() != true);
	if(!(*it)->isCancled())
	  {
	    res.push_back((*it)->getTask());
	    if((*it)->getTimerType() == Timer::TimerType::LOOP)
	      {
		(*it)->setNextExpireTime();
 		minExpireTime = std::min(minExpireTime, (*it)->_nxtExpireTime);
		tms.push_back(*it);
	      }
	  }
      }
    _timers.erase(_timers.begin(),pos);
    //循环定时器
    for(auto item : tms) _timers.insert(item);
    //LOG_ROOT_INFO() << "res size : " <<res.size() << "cur : "<<t->_nxtExpireTime;
    return res;
  }

  uint64_t TimerManager::getNextExpireTime()
  {
    RDLockGuard lg(_mx);
    if(_timers.empty()) return (uint64_t)-1;
    auto it =  _timers.begin();
    return (*it)->_nxtExpireTime;
  }

  uint64_t TimerManager::getNextExpireInterval()
  {
    auto cur = Timer::getCurrentTime();
    auto nxt = getNextExpireTime();
    if( nxt == (uint64_t)-1)
      return (uint64_t)-1;
    //ASSERT2(nxt >= cur,"Current : "+std::to_string(cur)+" Next : "+std::to_string(nxt));
    return nxt >= cur ? nxt - cur : 0;
  }

  int TimerManager::addTimer(Timer::ptr timer,
			     std::function<void()> cb,
			     Timer::TimerType type)
  {
    ASSERT(timer != nullptr);
    bool headTag = false;
    WRLockGuard lg(_mx);
    //如果是循环定时器更改定时器类型
    if(type == Timer::TimerType::LOOP) timer->setLoop();
    //设置本Manager为定时器Manager
    timer->_cancled = false;
    timer->setManager(this);
    //设置任务
    timer->setTask(cb);
    _timers.insert(timer);
    if(_timers.find(timer) == _timers.begin()) headTag = true;
    timer->start();
    lg.unlock();
    if(headTag)
      {
	notify();
      }
    return 0;
  }
  int TimerManager::addTimer(Timer::ptr* timer,
			     std::function<void()> cb,
			     Timer::TimerType type)
  {
    ASSERT(timer != nullptr);
    Timer::ptr t = nullptr;
    t.swap(*timer);
    return addTimer(t,cb,type);
  }
  
}
