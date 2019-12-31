#include "IOManager.hpp"
#include "../macro.hpp"

namespace zhuyh
{ 
  static Logger::ptr sys_log = GET_LOGGER("system");

  IOManager::IOManager(const std::string& name,Scheduler* scheduler )
  {
    _epfd = epoll_create(1);
    if(_epfd < 0 )
      {
	throw std::logic_error("epoll_create error");
      }
    if(scheduler == nullptr)
      _scheduler = Scheduler::getThis();
    else
      _scheduler = scheduler;
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
    ASSERT(rt >= 0);
    _thread.reset(new Thread(std::bind(&IOManager::run,this),_name));
    //LOG_INFO(sys_log) << "IOManager : "<< _name <<" Created!";
  }
  IOManager::~IOManager()
  {
    if(!_stopping)
      stop();
    LOG_INFO(sys_log) << "IOManager Destroyed";
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
	delete p.second;
	p.second = nullptr;
      }
    delete _notifyEvent;
    _eventMap.clear();
  }
  void IOManager::notify()
  {
    int rt = write(_notifyFd[1],"",1);
    ASSERT(rt >= 0);
  }

  int IOManager::addEvent(int fd,Task::ptr task,EventType type)
  {
    ASSERT( type == READ  || type == WRITE);
    WRLockGuard lg(_lk);
    struct epoll_event ev;
    FdEvent*& epEv = _eventMap[fd];
    LOG_INFO(sys_log) << "fd = "<<fd;
    if(epEv == nullptr)
      {
	epEv = new FdEvent(fd,NONE);
      }
    lg.unlock();
    LockGuard lg2(epEv->lk);
    int rt = setNonb(fd);
    if(rt < 0)
      {
	LOG_ERROR(sys_log) << "Failed to setNonb";
	return -1;
      }
    auto op = epEv->event == NONE ? EPOLL_CTL_ADD : EPOLL_CTL_MOD;
    if(type & epEv->event)
      {
	LOG_WARN(sys_log) << "type : " << type << " Exist"
			  << "current : " << epEv->event;
	return false;
      }
    ev.events = epEv->event | EPOLLET | type;
    ev.data.ptr = epEv;
    rt = 0;
    rt = epoll_ctl(_epfd,op,fd,&ev);
    if(rt)
      {
	LOG_ERROR(sys_log) << "epoll_ctl errro : " << strerror(errno);
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
    WRLockGuard lg(_lk);
    FdEvent*& epEv = _eventMap[fd];
    if(epEv == nullptr)
      {
	LOG_WARN(sys_log) << "Failed to del Event";
	return -1;
      }
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
    WRLockGuard lg(_lk);
    FdEvent*& epEv = _eventMap[fd];
    if(epEv == nullptr)
      {
	LOG_WARN(sys_log) << "event does not exist";
	return -1;
      }
    lg.unlock();
      
    LockGuard lg2(epEv->lk);
    if( (EventType)(epEv->event & type) == NONE )
      {
	LOG_WARN(sys_log) << "type : " << type << " Not Exist";
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
	return -1;
      }
    triggerEvent(epEv,type);
    --_holdCount;
    return 0;
  }

  int IOManager::cancleAll(int fd)
  {
    WRLockGuard lg(_lk);
    FdEvent*& epEv = _eventMap[fd];
    if(epEv == nullptr )
      {
	LOG_WARN(sys_log) << "event doesn't exist";
	return -1;
      }
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
    Fiber::getThis();
    const int MaxEvent = 1000;
    struct epoll_event* events = new epoll_event[1000];
    //毫秒
    const int MaxTimeOut = 500;
    while(1)
      {
	LOG_INFO(sys_log) << "Holding : " << _holdCount
			  << " Total  : " << _scheduler->totalTask;
	if(isStopping())
	  {
	    LOG_INFO(sys_log) << "IOManager : " << _name << " stopped!";
	    delete [] events;
	    break;
	  }
	int rt = 0;
	do{
	  rt = epoll_wait(_epfd,events,MaxEvent,MaxTimeOut);
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
	for(int i=0;i<rt;i++)
	  {
	    struct epoll_event& ev = events[i];
	    FdEvent* epEv = (FdEvent*)ev.data.ptr;
	    if(epEv->fd == _notifyFd[0]) continue;
	    int real_event = NONE;
	    LockGuard lg(epEv->lk);
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
	    if(epEv->timer != nullptr && epEv->timer->getTimerType() == Timer::LOOP)
	      {
		ASSERT(false);
	      }
	    else
	      {
		int op = (tevent == NONE) ? EPOLL_CTL_DEL : EPOLL_CTL_MOD;
		ev.events = tevent | EPOLLET;
		int rt2 = epoll_ctl(_epfd,op,epEv->fd,&ev);
		if(rt2)
		  {
		    LOG_ERROR(sys_log) << "epoll_ctl error";
		    continue;
		  }
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
  
  Scheduler* IOManager::getScheduler()
  {
    return nullptr;
  }

  void IOManager::setScheduler(Scheduler* scheduler)
  {
  }

  int IOManager::triggerEvent(FdEvent* epEv,EventType type)
  {
    ASSERT( type == READ  || type == WRITE);
    ASSERT(type & epEv->event);
    epEv->event =(EventType)(epEv-> event & ~type);
    if(type & READ)
      {
	ASSERT(epEv->rdtask != nullptr);
	if(epEv->rdtask->fiber)
	  while(epEv->rdtask->fiber->_state != Fiber::HOLD)
	    ;
	if(epEv->timer != nullptr)
	  {
	    if(epEv->timer->getTimerType() == Timer::SINGLE)
	      epEv->timer.reset();
	  }
	Task::ptr task = nullptr;
	task.swap(epEv->rdtask);
	_scheduler->addTask(task);
      }
    else if(type & WRITE)
      {
	if(epEv->wrtask->fiber)
	  while(epEv->wrtask->fiber->_state != Fiber::HOLD)
	    ;
	Task::ptr task = nullptr;
	task.swap(epEv->wrtask);
	ASSERT(epEv->timer == nullptr);
	_scheduler->addTask(task);
      }
    return 0;
  }
  
  int IOManager::delTimer(int fd)
  {
    WRLockGuard lg(_lk);
    FdEvent* epEv = _eventMap[fd];
    if(epEv == nullptr) return -1;
    if(epEv-> timer == nullptr) return -1;
    lg.unlock();
    
    LockGuard lg2(epEv->lk);
    ASSERT(epEv->event == READ);
    int rt = epoll_ctl(_epfd,EPOLL_CTL_DEL,fd,nullptr);
    if(rt)
      {
	LOG_ERROR(sys_log) << "epoll ctl error : "<<strerror(errno);
	return -1;
      }
    epEv->event = NONE;
    epEv->timer.reset();
    --_holdCount;
    return 0;
  }
  
};
