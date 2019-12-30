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
#include "../logUtil.hpp"
#include "TimerManager.hpp"
#include "../macro.hpp"
namespace zhuyh
{
  //static Logger::ptr sys_log = GET_LOGGER("system");
  class Scheduler;
  struct Task;
  class IOManager final
  {
  public:
    friend class Scheduler;
    enum EventType
      {
	NONE = 0x0,
	READ = EPOLLIN,
	WRITE = EPOLLOUT
      };
    struct FdEvent final
    {
      
      typedef std::shared_ptr<FdEvent> ptr;
      FdEvent(int _fd,EventType _event)
      {
	fd = _fd;
	event = _event;
      }
      FdEvent() {}
    public:
      Mutex lk;
      int fd;
      Timer::ptr timer = nullptr;
      EventType event = NONE;
      std::shared_ptr<Task> rdtask = nullptr;
      std::shared_ptr<Task> wrtask = nullptr;
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
    int triggerEvent(FdEvent* epEv,EventType type);
    template<class T>
    int addTimer(Timer::ptr* timer,T cb,
		 Timer::TimerType type = Timer::SINGLE);
    template<class T>
    int addTimer(Timer::ptr timer,T cb,
	     Timer::TimerType type = Timer::SINGLE);
    int delTimer(int fd);
    
  private:
    
    void notify();
    //epoll句柄
    int _epfd = -1;
    Scheduler* _scheduler;
    // fd --> event
    std::unordered_map<int,FdEvent*> _eventMap;
    mutable RWLock _lk;
    int _notifyFd[2];
    Thread::ptr _thread = nullptr;
    std::string _name;
    std::atomic<bool> _stopping{false};
    std::atomic<int> _holdCount{0};
    FdEvent* _notifyEvent;
  };  
  
  template<class T>
  int IOManager::addTimer(Timer::ptr timer,T cb,
			  Timer::TimerType type) 
  {
    //LOG_ROOT_INFO() << "adding1";
    if(timer == nullptr) ASSERT(0);
    if(type == Timer::LOOP)
      timer->setLoop();
    struct epoll_event ev;
    static Logger::ptr sys_log = GET_LOGGER("system");
    //TODO:改成std::vector来提高性能
    WRLockGuard lg(_lk);
    int tfd = timer->getTimerFd();
    ASSERT(tfd >= 0);
    FdEvent*& epEv = _eventMap[tfd];
    if(epEv == nullptr)
      {
	epEv = new FdEvent(tfd,NONE);
      }
    else if(epEv->timer != nullptr)
      {
	ASSERT(false);
	return -1;
      }
    ASSERT2(epEv->event == NONE,std::to_string(int(epEv->event)));
    lg.unlock();
    LockGuard lg2(epEv->lk);
    //定时器一定是一个读时间
    ev.events = EPOLLIN | EPOLLET;
    ev.data.ptr = epEv;
    int rt = epoll_ctl(_epfd,EPOLL_CTL_ADD,tfd,&ev);
    if(rt < 0)
      {
	timer.reset();
	ASSERT(false);
	LOG_ERROR(sys_log) << "Failed to add timer";
	return -1;
      }
    //LOG_INFO(sys_log) << "add a timer";
    epEv -> timer = timer;
    epEv -> rdtask.reset(new Task(cb));
    //LOG_INFO(sys_log) << "ADD TIMER";
    epEv -> event = (EventType)(epEv->event | READ);
    try
      {
	timer->start();
      }
    catch(std::exception& e)
      {
	LOG_ERROR(sys_log) << e.what();
	return -1;
      }
    ++_holdCount;
    //LOG_ROOT_INFO() << "adding3";
    //LOG_INFO(sys_log) << "ADD";
    return 0;
  }
  template<class T>
  int IOManager::addTimer(Timer::ptr* timer,T cb,
			  Timer::TimerType type)
  {
    if(timer == nullptr) return -1;
    Timer::ptr t;
    t.swap(*timer);
    return addTimer(t,cb,type);
  }
}
