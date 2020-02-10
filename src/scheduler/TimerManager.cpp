#include "TimerManager.hpp"
#include "../macro.hpp"
#include "../logUtil.hpp"
#include "Task.hpp"
#include <list>
#include <vector>
#include <algorithm>
namespace zhuyh
{
  //std::atomic<uint64_t> Timer::__id__{0};
  static Logger::ptr sys_log = GET_LOGGER("system");
  Timer::Timer(time_t sec,time_t msec ,time_t usec ,time_t nsec)
  {
    //_id = ++__id__;
    _cancled = false;
    ASSERT(sec >= 0 && msec >= 0 && usec >= 0 && nsec >= 0);
    usec += nsec/1000;
    msec += usec/1000;
    _interval = ((uint64_t)msec)+((uint64_t)sec)*1000ull;
    //开机时间
    _nxtExpireTime = _interval + getCurrentTime();
    //LOG_ROOT_INFO() << "nxt : "<<_nxtExpireTime;
    _type = SINGLE;
    _manager = nullptr;
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
    LockGuard lg(_manager->_mx);
    if(_cancled) return false;
    if(_task)
      {
	_cancled = true;
	auto self = shared_from_this();
	auto it = _manager->_timers.find(self);
	//LOG_ROOT_INFO() << (unsigned long long)self.get();
	//if(it != _manager->_timers.end())
	//coredump here
	_manager->_timers.erase(it);
	return true;
      }
    return false;
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
    if(!o1 && !o2) return false;
    if(!o1) return true;
    if(!o2) return false;
    if(o1->_nxtExpireTime < o2->_nxtExpireTime)
      return true;
    if(o1->_nxtExpireTime > o2->_nxtExpireTime)
      return false;
    return o1.get() < o2.get();
  }
  std::list<Task::ptr> TimerManager::getExpiredTasks()
  {
    std::list<Task::ptr> res;
    std::list<Timer::ptr> tms;
    LockGuard lg(_mx);
    //LOG_ROOT_INFO() << "total : "<<_timers.size();
    if(_timers.empty() ) return res;
    auto t = Timer::ptr(new Timer());
    auto pos = _timers.begin();
    while(pos != _timers.end() && (*pos)->_nxtExpireTime <= t->_nxtExpireTime)
      {
	pos++;
      }
    tms.insert(tms.begin(),_timers.begin(),pos);
    _timers.erase(_timers.begin(),pos);
    for(auto& item : tms)
      {
	res.push_back(item->_task);
	if(item->_type == Timer::LOOP)
	  {
	    _timers.insert(item);
	  }
	else
	  {
	    item->_task = nullptr;
	  }
      }
    //LOG_ROOT_INFO() << "res size : " <<res.size() << "cur : "<<t->_nxtExpireTime;
    return res;
  }

  uint64_t TimerManager::getNextExpireTime()
  {
    LockGuard lg(_mx);
    if(_timers.empty()) return (uint64_t)-1;
    auto it =  _timers.begin();
    //coredump here
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
    LockGuard lg(_mx);
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
