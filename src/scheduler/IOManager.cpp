#include "IOManager.hpp"
#include <list>
#include "../config.hpp"
#include "Scheduler.hpp"
#include <algorithm>
#include "../netio/Hook.hpp"

namespace zhuyh
{ 
  static Logger::ptr sys_log = GET_LOGGER("system");
  static int setNonb(int fd)
  {
    int flags = fcntl_f(fd,F_GETFL);
    if(flags < 0 )
      return -1;
    return fcntl_f(fd,F_SETFL,flags | O_NONBLOCK);
  }
  
  static int clearNonb(int fd)
  {
    int flags = fcntl_f(fd,F_GETFL);
    if(flags < 0 )
      return -1;
    return fcntl_f(fd,F_SETFL,flags & ~O_NONBLOCK);
  }
  IOManager::IOManager(const std::string& name)
  {
    _epfd = epoll_create(1);
    if(_epfd < 0 )
      {
	throw std::logic_error("epoll_create error");
      }
    _scheduler = Scheduler::Schd::getInstance();
    if(name == "")
      _name = "IOManager";
    else
      _name = name;
    int rt = pipe(_notifyFd);
    ASSERT2(rt >= 0,strerror(errno));
    setNonb(_notifyFd[0]);
    setNonb(_notifyFd[1]);
    struct epoll_event ev;
    _notifyEvent = new FdEvent();
    _notifyEvent->fd = _notifyFd[0];
    ev.events = EPOLLIN | EPOLLET;
    ev.data.ptr = _notifyEvent;
    rt = epoll_ctl(_epfd,EPOLL_CTL_ADD,_notifyFd[0],&ev);
    resizeMap((uint32_t)4096);
    ASSERT(rt >= 0);
    _thread.reset(new Thread(std::bind(&IOManager::run,this),_name));
    //LOG_INFO(sys_log) << "IOManager : "<< _name <<" Created!";
  }
  IOManager::~IOManager()
  {
    if(!_stopping)
      stop();
    //LOG_INFO(sys_log) << "IOManager Destroyed";
    if(_epfd >= 0)
      {
	close(_epfd);
	_epfd = -1;
      }
    if(_notifyFd[0] != -1)
      {
	close(_notifyFd[0]);
	_notifyFd[0]=-1;
      }
    if(_notifyFd[1] != -1)
      {
	close(_notifyFd[1]);
	_notifyFd[1] = -1;
      }
    _thread->join();
    for(auto  p : _eventMap)
      {
	if(p)
	  {
	    delete p;
	    p = nullptr;
	  }
      }
    delete _notifyEvent;
    _eventMap.clear();
  }
  void IOManager::notify()
  {
    int rt = write(_notifyFd[1],"",1);
    ASSERT(rt >= 0);
  }
  void IOManager::resizeMap(uint32_t size)
  {
    _eventMap.resize(size);
    for(size_t i=0;i<_eventMap.size();i++)
      {
	if(_eventMap[i] == nullptr)
	  {
	    _eventMap[i] = new FdEvent(i,NONE);
	  }
      }
  }
  int IOManager::addEvent(int fd,Task::ptr task,EventType type)
  {
    ASSERT( type == READ  || type == WRITE);
    RDLockGuard lg(_lk);
    struct epoll_event ev;
    //TODO : 改为vector
    FdEvent* epEv = nullptr;
    ASSERT2(fd >= 0,"fd cannot be smaller than 0");
    //LOG_INFO(sys_log) << "fd = "<<fd;
    if((size_t)fd < _eventMap.size())
      {
	epEv = _eventMap[fd];
	lg.unlock();
      }
    else
      {
	lg.unlock();
	WRLockGuard lg3(_lk);
	resizeMap(_eventMap.size()*2);
	epEv = _eventMap[fd];
      }
    LockGuard lg2(epEv->lk);
    int rt = setNonb(fd);
    if(rt < 0)
      {
	//ASSERT(0);
	LOG_ERROR(sys_log) << "Failed to setNonb";
	return -1;
      }
    auto op = epEv->event == NONE ? EPOLL_CTL_ADD : EPOLL_CTL_MOD;
    if(type & epEv->event)
      {
	//ASSERT(0);
	LOG_ERROR(sys_log) << "type : " << type << " Exist"
			   << "current : " << epEv->event
			   <<" fd = " <<fd;
	return -1;
      }
    ev.events = epEv->event | EPOLLET | type;
    ev.data.ptr = epEv;
    rt = 0;
    rt = epoll_ctl(_epfd,op,fd,&ev);
    if(rt)
      {
	LOG_ERROR(sys_log) << "epoll_ctl errro : " << strerror(errno);
	//ASSERT2(false,strerror(errno));
	return -1;
      }
    if(type & EventType::READ)
       {
	 ASSERT(epEv->rdtask == nullptr);
	 epEv->rdtask = task;
	 epEv->event = (EventType)(epEv->event | EventType::READ);
      }
    else  if(type & EventType::WRITE)
      {
	ASSERT(epEv->wrtask == nullptr);
	epEv->wrtask = task;
	epEv->event = (EventType)(epEv->event | EventType::WRITE);
      }
    ++_holdCount;
    return 0;
  }

  int IOManager::delEvent(int fd,EventType type)
  {
    ASSERT( !((type & READ) && (type & WRITE))  );
    ASSERT( type != NONE);
    ASSERT(type == READ || type == WRITE);
    struct epoll_event ev;
    RDLockGuard lg(_lk);
    if((size_t)fd >= _eventMap.size())
      {
	LOG_ERROR(sys_log) << "fd<"<<fd<<"> is bigger than _eventMap.size()<"<<_eventMap.size()<<">";
	return -1;
      }
    FdEvent* epEv = _eventMap[fd];
    lg.unlock();
    
    LockGuard lg2(epEv->lk);
    if( (EventType)(type & epEv->event) == NONE )
      {
	LOG_WARN(sys_log) << "type : " << type << " Not Exist";
	return -1;
      }
    auto tevent = ~type & epEv->event;
    auto op =  tevent ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    ev.events = tevent | EPOLLET;
    ev.data.ptr = epEv;
    int rt = epoll_ctl(_epfd,op,fd,&ev);
    if(rt)
      {
	LOG_ERROR(sys_log) << "epoll_ctl error";
	return -1;
      }
    epEv->event = (EventType)tevent;
    if(type & READ)
      {
	epEv->rdtask.reset();
      }
    else if(type & WRITE)
      {
	epEv->wrtask.reset();
      }
    --_holdCount;
    return 0;
  }

  int IOManager::cancleEvent(int fd,EventType type)
  {
    ASSERT( !((type & READ) && (type & WRITE))  );
    ASSERT( type != NONE);
    ASSERT(type == READ || type == WRITE);
    struct epoll_event ev;
    RDLockGuard lg(_lk);
    if((size_t)fd >= _eventMap.size())
      {
	LOG_WARN(sys_log) << "fd<"<<fd<<"> is bigger than _eventMap.size()<"<<_eventMap.size()<<">";
	return -1;
      }
    FdEvent* epEv = _eventMap[fd];
    lg.unlock();
    LockGuard lg2(epEv->lk);
    if( (EventType)(epEv->event & type) == NONE )
      {
	//LOG_WARN(sys_log) << "type : " << type << " Not Exist";
	return -1;    	
      }
    auto tevent = epEv->event & ~type;
    auto op = tevent ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    ev.events = EPOLLET | tevent;
    ev.data.ptr = epEv;
    int rt = epoll_ctl(_epfd,op,fd,&ev);
    if(rt)
      {
	LOG_ERROR(sys_log) << "epoll_ctl error";
	return -2;
      }
    triggerEvent(epEv,type);
    --_holdCount;
    return 0;
  }

  int IOManager::cancleAll(int fd)
  {
    RDLockGuard lg(_lk);
    if((size_t)fd >= _eventMap.size())
      {
	LOG_WARN(sys_log) << "fd<"<<fd<<"> is bigger than _eventMap.size()<"<<_eventMap.size()<<">";
	return -1;
      }
    FdEvent* epEv = _eventMap[fd];
    lg.unlock();
    
    LockGuard lg2(epEv->lk);
    if(epEv->event == NONE) return false;
    int rt = epoll_ctl(_epfd,EPOLL_CTL_DEL,fd,nullptr);
    if(rt)
      {
	LOG_ERROR(sys_log) << "epoll_ctl error";
	return -1;
      }
    
    if(epEv->event & READ)
      {
	triggerEvent(epEv,READ);
	--_holdCount;
      }
    if(epEv->event & WRITE)
      {
        triggerEvent(epEv,WRITE);
	--_holdCount;
      }
    return 0;
  }

  bool IOManager::isStopping() const
  {
    return _holdCount == 0 && _stopping && _scheduler->totalTask == 0;
  }
  
  void IOManager::stop()
  {
    _stopping = true;
    notify();
  }
  
  void IOManager::run()
  {
    Hook::setHookState(true);
    Fiber::getThis();
    const int MaxEvent = 1000;
    struct epoll_event* events = new epoll_event[1001];
    //毫秒
    const int MaxTimeOut = 500;
    while(1)
      {
	//LOG_INFO(sys_log) << "Holding : " << _holdCount << " Total  : " << _scheduler->totalTask;
	if(isStopping())
	  {
	    //LOG_INFO(sys_log) << "IOManager : " << _name << " stopped!";
	    delete [] events;
	    break;
	  }
	int rt = 0;
	int nxtTimeOut = (int)std::min(getNextExpireInterval(),(uint64_t)MaxTimeOut);
	do{
	  rt = epoll_wait(_epfd,events,MaxEvent,nxtTimeOut);
	  if(rt<0 && errno == EINTR)
	    {
	      LOG_INFO(sys_log) <<"EINTR";
	      ;
	    }
	  else
	    {
	      break;
	    }
	}while(1);
	std::list<Task::ptr> tasks = getExpiredTasks();
	for(Task::ptr& item : tasks)
	  {
	    _scheduler->addTask(item);
	  }
	for(int i=0;i<rt;i++)
	  {
	    struct epoll_event& ev = events[i];
	    FdEvent* epEv = (FdEvent*)ev.data.ptr;
	    if(epEv->fd == _notifyFd[0]) continue;
	    int real_event = NONE;
	    LockGuard lg(epEv->lk);
	    if(ev.events & (EPOLLERR | EPOLLHUP))
	      {
		ev.events |= (EPOLLIN | EPOLLOUT) & (epEv->event);
	      }
	    if(ev.events & READ )
	      {
		real_event |= READ;
	      }
	    if(ev.events & WRITE)
	      {
		real_event |= WRITE;
	      }
	    if( (real_event & epEv->event) == NONE) continue;
	    
	    EventType tevent = (EventType)(~real_event & epEv->event);
	    
	    int op = (tevent == NONE) ? EPOLL_CTL_DEL : EPOLL_CTL_MOD;
	    ev.events = tevent | EPOLLET;
	    int rt2 = epoll_ctl(_epfd,op,epEv->fd,&ev);
	    if(rt2)
	      {
		LOG_ERROR(sys_log) << "epoll_ctl error";
		continue;
	      }
	    if(real_event & READ)
	      {
		triggerEvent(epEv,READ);
		--_holdCount;
	      }
	    if(real_event & WRITE)
	      {
		triggerEvent(epEv,WRITE);
		--_holdCount;
	      }
	  }
      }
  }

  void IOManager::clearAllEvent()
  {
  }

  int IOManager::triggerEvent(FdEvent* epEv,EventType type)
  {
    //ASSERT2(0, "Trigger event");
    ASSERT( type == READ  || type == WRITE);
    ASSERT(type & epEv->event);
    epEv->event =(EventType)(epEv-> event & ~type);
    if(type & READ)
      {
	//LOG_ROOT_INFO() << "Triggle READ Event";
	ASSERT(epEv->rdtask != nullptr);
	Task::ptr task = nullptr;
	task.swap(epEv->rdtask);
	_scheduler->addTask(task);
      }
    else if(type & WRITE)
      {
	//LOG_ROOT_INFO() << "Triggle WRITE Event";
	Task::ptr task = nullptr;
	task.swap(epEv->wrtask);
	//LOG_ROOT_INFO() << "added task"<<(unsigned long long)task.get();
	_scheduler->addTask(task);
      }
    return 0;
  }
};
