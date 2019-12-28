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
    _notifyEvent.reset(new FdEvent());
    _notifyEvent->fd = _notifyFd[0];
    ev.events = EPOLLIN | EPOLLET;
    ev.data.ptr = _notifyEvent.get();
    rt = epoll_ctl(_epfd,EPOLL_CTL_ADD,_notifyFd[0],&ev);
    ASSERT(rt >= 0);
    _thread.reset(new Thread(std::bind(&IOManager::run,this),_name));
    LOG_DEBUG(sys_log) << "IOManager : "<< _name <<" Created!";
  }
  IOManager::~IOManager()
  {
    if(!_stopping)
      stop();
    LOG_DEBUG(sys_log) << "IOManager Destroyed";
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
    _eventMap.clear();
    _thread->join();
  }
  void IOManager::notify()
  {
    int rt = write(_notifyFd[1],"",1);
    ASSERT(rt >= 0);
  }
  //增加一个addEvent对外界使用,该方法变为私有
  int IOManager::addEvent(int fd,Task::ptr task,EventType type)
  {
    ASSERT( type == READ  || type == WRITE);
    RDLockGuard lg(_lk);
    struct epoll_event ev;
    FdEvent::ptr& epEv = _eventMap[fd];
    if(epEv == nullptr)
      {
	//	LOG_DEBUG(sys_log) << "HERE4";
	lg.unlock();
	WRLockGuard t(_lk);
	epEv.reset(new FdEvent(fd,NONE));
      }
    else
      {
	lg.unlock();
      }
    //LOG_DEBUG(sys_log) << "HERE2";
    LockGuard lg2(epEv->lk);
    int rt = setNonb(fd);
    if(rt < 0)
      {
	LOG_ERROR(sys_log) << "Failed to setNonb";
	return -1;
      }
    //LOG_DEBUG(sys_log) << "HERE3";
    auto op = epEv->event == NONE ? EPOLL_CTL_ADD : EPOLL_CTL_MOD;
    //添加之前应该无改属性
    if(type & epEv->event)
      {
	LOG_WARN(sys_log) << "type : " << type << " Exist"
			  << "current : " << epEv->event;
	return false;
      }
    ev.events = epEv->event | EPOLLET | type;
    ev.data.ptr = epEv.get();
    rt = 0;
    rt = epoll_ctl(_epfd,op,fd,&ev);
    if(rt)
      {
	LOG_ERROR(sys_log) << "epoll_ctl errro : " << strerror(errno);
	return -1;
      }
    if(type & EventType::READ)
       {
	 //无读事件
	 ASSERT(epEv->rdtask == nullptr);
	 epEv->rdtask = task;
	 epEv->event = (EventType)(epEv->event | EventType::READ);
	 //LOG_DEBUG(sys_log) << "ADD READ EVENT";
      }
    else  if(type & EventType::WRITE)
      {
	ASSERT(epEv->wrtask == nullptr);
	epEv->wrtask = task;
	epEv->event = (EventType)(epEv->event | EventType::WRITE);
	//LOG_DEBUG(sys_log) << "ADD WRITE EVENT";
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
    FdEvent::ptr epEv = _eventMap[fd];
    if(epEv == nullptr) return -1;
    lg.unlock();
    //LOG_INFO(sys_log) << "HERE";
    LockGuard lg2(epEv->lk);
    if( (EventType)(type & epEv->event) == NONE )
      {
	LOG_WARN(sys_log) << "type : " << type << " Not Exist";
	return -1;
      }
    auto tevent = ~type & epEv->event;
    auto op =  tevent ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    ev.events = tevent | EPOLLET;
    //???
    ev.data.ptr = epEv.get();
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
    FdEvent::ptr epEv;
    RDLockGuard lg(_lk);
    epEv = _eventMap[fd];
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
    ev.data.ptr = epEv.get();
    int rt = epoll_ctl(_epfd,op,fd,&ev);
    if(rt)
      {
	LOG_ERROR(sys_log) << "epoll_ctl error";
	return -1;
      }
    triggerEvent(epEv.get(),type);
    --_holdCount;
    return 0;
  }

  int IOManager::cancleAll(int fd)
  {
    FdEvent::ptr epEv;
    RDLockGuard lg(_lk);
    epEv = _eventMap[fd];
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
	triggerEvent(epEv.get(),READ);
	--_holdCount;
      }
    if(epEv->event & WRITE)
      {
        triggerEvent(epEv.get(),WRITE);
	--_holdCount;
      }
    return 0;
  }

  bool IOManager::isStopping() const
  {
    return _holdCount == 0 && _stopping;
  }
  
  void IOManager::stop()
  {
    _stopping = true;
    notify();
  }
  
  void IOManager::run()
  {
    Fiber::getThis();
    const int MaxEvent = 256;
    std::vector<struct epoll_event> events(256);
    //毫秒
    const int MaxTimeOut = 500;
    while(1)
      {
	if(isStopping())
	  {
	    LOG_DEBUG(sys_log) << "IOManager : " << _name << " stopped!";
	    break;
	  }
	int rt = 0;
	do{
	  //LOG_INFO(sys_log) << "Here";
	  rt = epoll_wait(_epfd,&*events.begin(),MaxEvent,MaxTimeOut);
	  //LOG_INFO(sys_log) << "Here2";
	  //被中断打断
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
	      ;
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
	    //	    epEv->event = tevent;
	    if(real_event & READ)
	      {
		//LOG_DEBUG(sys_log) << "Triggled Read Event";
		triggerEvent(epEv,READ);
		if(!(epEv->timer != nullptr && epEv->timer->getTimerType() == Timer::LOOP))
		  {
		    --_holdCount;
		  }
	      }
	    if(real_event & WRITE)
	      {
		//LOG_DEBUG(sys_log) << "Triggled Write Event";
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
    _scheduler = scheduler;
  }

  int IOManager::triggerEvent(FdEvent* epEv,EventType type)
  {
    ASSERT( type == READ  || type == WRITE);
    ASSERT(type & epEv->event);
    //epEv->event =(EventType)(epEv-> event & ~type);
    if(type & READ)
      {
	Task::ptr task = epEv->rdtask;
	//目的是关闭fd
	if(epEv->timer != nullptr)
	  {
	    char c;
	    read(epEv->fd,&c,1);
	    
	    if(epEv->timer->getTimerType() == Timer::SINGLE)
	      epEv->timer.reset();
	  }
	else
	  epEv->rdtask.reset();
	_scheduler->addTask(task);
      }
    else if(type & WRITE)
      {
	Task::ptr task = epEv->wrtask;
	ASSERT(epEv->timer == nullptr);
	epEv->wrtask.reset();
	_scheduler->addTask(task);
      }
    epEv->event =(EventType)(epEv-> event & ~type);
    return 0;
  }

  //Timer是一个读事件
  int IOManager::addTimer(Timer::ptr timer,std::function<void()> cb,
			   Timer::TimerType type) 
  {
    if(timer == nullptr) return -1;
    if(type == Timer::LOOP)
      timer->setLoop();
    int tfd = timer->getTimerFd();
    ASSERT(tfd >= 0);
    struct epoll_event ev;
    
    RDLockGuard lg(_lk);
    FdEvent::ptr& epEv = _eventMap[tfd];
    if(epEv == nullptr)
      {
	lg.unlock();
	WRLockGuard t(_lk);
	epEv.reset(new FdEvent(tfd,NONE));
      }
    else
      {
	return -1;
      }
    LockGuard lg2(epEv->lk);
    //定时器一定是一个读时间
    ev.events = EPOLLIN | EPOLLET;
    ev.data.ptr = epEv.get();
    int rt = epoll_ctl(_epfd,EPOLL_CTL_ADD,tfd,&ev);
    if(rt < 0)
      {
	LOG_ERROR(sys_log) << "Failed to add timer";
	return -1;
      }
    
    //LOG_INFO(sys_log) << "add a timer";
    epEv -> timer = timer;
    epEv -> rdtask.reset(new Task(cb));
    epEv -> event = (EventType)(epEv->event | READ);
    timer->start();
    ++_holdCount;
    return 0;
  }

  int IOManager::addTimer(Timer::ptr* timer,std::function<void()> cb,
			   Timer::TimerType type)
  {
    if(timer == nullptr) return -1;
    Timer::ptr t;
    t.swap(*timer);
    return addTimer(t,cb,type);
  }
  int IOManager::delTimer(int fd)
  {
    RDLockGuard lg(_lk);
    FdEvent::ptr epEv = _eventMap[fd];
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
