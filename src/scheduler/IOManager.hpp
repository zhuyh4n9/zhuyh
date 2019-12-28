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

namespace zhuyh
{
  class Scheduler;
  struct Task;
  class IOManager final : public TimerManager
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
    
    int addTimer(Timer::ptr* timer,std::function<void()> cb,
		 Timer::TimerType type = Timer::SINGLE) override;
    int addTimer(Timer::ptr timer,std::function<void()> cb,
		 Timer::TimerType type = Timer::SINGLE) override;
    int delTimer(int fd) override;
    
  private:
    
    void notify();
    //epoll句柄
    int _epfd = -1;
    Scheduler* _scheduler;
    // fd --> event
    std::unordered_map<int,FdEvent::ptr> _eventMap;
    mutable RWLock _lk;
    int _notifyFd[2];
    Thread::ptr _thread = nullptr;
    std::string _name;
    std::atomic<bool> _stopping{false};
    std::atomic<int> _holdCount{0};
    FdEvent::ptr _notifyEvent;
  };
  
}
