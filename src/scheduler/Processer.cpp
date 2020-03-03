#include "Processer.hpp"
#include "Scheduler.hpp"
#include <sys/epoll.h>
#include "../logUtil.hpp"
#include "../macro.hpp"
#include "../netio/Hook.hpp"

namespace zhuyh
{
  static Logger::ptr sys_log  = GET_LOGGER("system");
  thread_local Fiber::ptr __main_fiber = nullptr;
  std::atomic<int> Task::id{0};
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
  
  Processer::Processer(const std::string name)
    :_name(name)
  {
    _scheduler = Scheduler::Schd::getInstance();
    _epfd = epoll_create(1);
    ASSERT2(_epfd >=0 , "epoll_create error");
    int rt = pipe(_notifyFd);
    ASSERT2(rt >= 0,"pipe error");
    rt = setNonb(_notifyFd[0]);
    ASSERT2(rt >= 0 , "setNonb error");
    rt = setNonb(_notifyFd[1]);
    ASSERT2(rt >= 0 , "setNonb error");
    
    struct epoll_event ev{0};
    memset(&ev,0,sizeof(ev));
    ev.events = EPOLLET | EPOLLIN;
    ev.data.fd = _notifyFd[0];
    rt = epoll_ctl(_epfd,EPOLL_CTL_ADD,_notifyFd[0],&ev);
    ASSERT2(rt >= 0,"epoll_ctl error");
  }

  Processer::~Processer()
  {
    if(!_stopping)
      stop();
    if(_epfd != -1)
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
    //LOG_INFO(sys_log) << "Processer : "<<_name<<"  Destroyed, worked = :"<<worked;
  }
  
  void Processer::start()
  {
    try
      {
	_thread.reset(new Thread(std::bind(&Processer::run,shared_from_this()),_name));
      }
    catch (std::exception& e)
      {
	LOG_ERROR(sys_log) << e.what();
      }
    //LOG_INFO(sys_log) << "Processer Started!";
  }

  bool Processer::addTask(Task::ptr task)
  {
    ASSERT(_scheduler != nullptr);
    if(task == nullptr) return false;
    if(task->cb || task -> fiber)
      {
	if(task->cb)
	  {
	    ASSERT(task -> fiber == nullptr);
	    ++(_scheduler->totalTask);
	  }
	//单线程导致死循环
	// else if(task->fiber->_state != Fiber::READY)
	//   {
	//     //不是READY一定是要切换为HOLD
	//     while(task->fiber->_state != Fiber::HOLD)
	//       ;
	//   }
	//LOG_ROOT_ERROR() << "add Success : "<<(unsigned long long)task;
	/*
	  else
	  {
	    ASSERT2(task->fiber->getState() == Fiber::READY
		    || task->fiber->getState() == Fiber::HOLD,
		    Fiber::getState(task->fiber->getState()));
		    
	  }
	*/
	ASSERT(task != nullptr);
	_readyTask.push_front(task);
	++_payLoad;
	notify();
	return true;
      }
    ASSERT(false);
    return false;
  }
  
  //swap操作将会使得*t变为空
  bool Processer::addTask(Task::ptr* t)
  {
    if(t == nullptr) return false;
    Task::ptr task;
    task.swap(*t);
    return addTask(task);
  }
  
  std::list<Task::ptr> Processer::steal(int k)
  {
    std::list<Task::ptr> tasks;
    if(_readyTask.try_popk_front(k,tasks) == true)
      _payLoad -= tasks.size();
    return tasks;
  }
  bool Processer::store(std::list<Task::ptr>& tasks)
  {
    _readyTask.pushk_front(tasks);
    _payLoad += tasks.size();
    return true;
  }

  void Processer::stop()
  {
    _stopping = 1;
    notify();
  }
  
  int Processer::notify()
  {
    //LOG_INFO(sys_log) << "Notifyed";
    return write(_notifyFd[1],"",1);
  }
  
  bool Processer::isStopping() const
  {
    ASSERT(getMainFiber() != nullptr);
    return ( _readyTask.empty()  && _stopping );
  }

  bool Processer::getStopping() const
  {
    return _stopping;
  }
  
  void Processer::run()
  {
    
    Hook::setHookState(true);
    std::vector<struct epoll_event> evs(20);
    setMainFiber(Fiber::getThis());
    //ASSERT(_readyTask.empty());
    while(1)
      {
	Task::ptr task;
	//LOG_INFO(sys_log) << " totalTask : "<<_readyTask.size();
	while(_readyTask.try_pop_back(task))
	  {
	    ASSERT(task != nullptr);
	    worked++;
	    if(task->fiber)
	      {
		if(task -> fiber->getState() != Fiber::HOLD &&
		   task -> fiber->getState() != Fiber::READY)
		  // ASSERT2(task -> fiber->getState() == Fiber::EXEC,
		  // 	  Fiber::getState(task->fiber->getState()));
		//Take Care,Maybe Bugs
		if(task -> fiber->getState() == Fiber::EXEC)
		  {
		    _readyTask.push_front(task);
		    continue;
		  }
		task->fiber -> setState(Fiber::READY);
	      }
	    else if(task->cb)
	      {
		task->fiber.reset(new Fiber(task->cb));
		task->fiber->setState(Fiber::READY);
		task->cb = nullptr;
	      }
	    else
	      {
		ASSERT(false);
	      }
	    Fiber::ptr& fiber = task->fiber;
	    //task.reset();
	    ASSERT(fiber != nullptr);
	    ASSERT(fiber->_stack != nullptr);
	    ASSERT2(fiber->getState() == Fiber::READY,Fiber::getState(fiber->getState()));
	    fiber->swapIn();
	    ASSERT(fiber->getState() != Fiber::INIT);
	    if(fiber->getState() == Fiber::READY)
	      {
		_readyTask.push_front(task);
	      }
	    else if(fiber->getState() == Fiber::EXEC)
	      {
		fiber->_state = Fiber::HOLD;
		--_payLoad;
	      }
	    else if(fiber->getState() == Fiber::HOLD)
	      {
		--_payLoad;
	      }
	    else if(fiber->getState() == Fiber::TERM
		    || fiber->getState() == Fiber::EXCEPT)
	      {
		--_payLoad;
		--(_scheduler->totalTask);
		fiber.reset();
	      }
	    else
	      {
		ASSERT2(false,Fiber::getState(fiber->getState()));
	      }
	  }
	//TOD:偷协程
	if(_scheduler->balance(shared_from_this()) > 0)
	  {
	    //LOG_INFO(sys_log) << "STOLED";
	    continue;
	  }
	if(_stopping)
	  {
	    if(_scheduler->totalTask <= 0
	       && _readyTask.empty()
	       && _scheduler->getHold() <= 0)
	      {
		return;
	      }
	  }
	
	static const int MaxTimeOut = 500;
	int rt = 0;
	while(1)
	  {
	    rt = epoll_wait(_epfd,&*evs.begin(),20,MaxTimeOut);
	    //被中断打断
	    if(rt < 0 && errno == EINTR)
	      {
		continue;
	      }
	    break;
	  }
	if(rt  < 0 )
	  {
	    LOG_ERROR(sys_log) << "epoll_wait error";
	    continue;
	  }
	char buf[64];
	rt = 0;
	while( (rt = read(_notifyFd[0],buf,63)) )
	  {
	    //LOG_DEBUG(sys_log) << "Recieved Notify! rt = "<<rt;
	    if(rt < 0)
	      {
		if(errno == EWOULDBLOCK || errno == EAGAIN )
		  break;
		LOG_ERROR(sys_log)<<"pipe read error";
		break;
	      }
	  }
	//LOG_DEBUG(sys_log) << "GOT HERE";
	if(_stopping)
	  {
	    //TODO:改为调度器holdCount
	    if(_scheduler->totalTask <= 0
	       && _readyTask.empty()
	       && _scheduler->getHold() <= 0)
	      {
		//LOG_INFO(sys_log) << "EXIT PROCESS";
		//退出run函数
		return;
	      }
	  }
      }
  }
  
  Fiber::ptr Processer::getMainFiber()
  {
    return __main_fiber;
  }
  
  void Processer::setMainFiber(Fiber::ptr fiber)
  {
    __main_fiber = fiber;
  }  
}
