#include "Processer.hpp"
#include <sys/epoll.h>

namespace zhuyh
{
  thread_local Fiber::ptr __main_fiber = nullptr;
  static Logger::ptr sys_log  = GET_LOGGER("system");
  
  Processer::Processer(const std::string name,Scheduler* scheduler )
    :_name(name)
  {
    //LOG_INFO(sys_log) << "Creating Processer";
    if(scheduler == nullptr )
      _scheduler = Scheduler::getThis();
    else
      _scheduler = scheduler;
    //创建管道和epoll对象,并且将管道读端放入epoll(ET模式)
    _epfd = epoll_create(1);
    ASSERT2(_epfd >=0 , "epoll_create error");
    int rt = pipe(_notifyFd);
    ASSERT2(rt >= 0,"pipe error");
    rt = setNonb(_notifyFd[0]);
    ASSERT2(rt >= 0 , "setNonb error");
    rt = setNonb(_notifyFd[1]);
    ASSERT2(rt >= 0 , "setNonb error");
    
    struct epoll_event ev;
    ev.events = EPOLLET | EPOLLIN;
    ev.data.fd = _notifyFd[0];
    rt = epoll_ctl(_epfd,EPOLL_CTL_ADD,_notifyFd[0],&ev);
    ASSERT2(rt >= 0,"epoll_ctl error");
    //LOG_INFO(sys_log) << "Processer created";
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
    LOG_DEBUG(sys_log) << "Processer Destroy";
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
  //只有主协程取任务执行是是从队列尾部取出
  //向就绪队列添加任务
  bool Processer::addTask(Task::ptr task)
  {
    
    if(task == nullptr) return false;
    else if(task->fiber)
      {
	task->fiber->setState(Fiber::READY);
	_readyTask.push_front(std::move(task->fiber));
	++_payLoad;
	notify();
	return true;
      }
    //是回调函数
    else if(task->cb)
      {
	Fiber::ptr fiber = nullptr;
	fiber.reset(new Fiber(task->cb));
	fiber->setState(Fiber::READY);
	_readyTask.push_front(std::move(fiber));
	++_payLoad;
	notify();
	return true;
      }
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
  
  //working steal算法
  std::list<Fiber::ptr> Processer::steal(int k)
  {
    std::list<Fiber::ptr> tasks;
    if(_readyTask.try_popk_front(k,tasks) == false);
      //LOG_INFO(sys_log) << "Failed to Steal Fibers";
    else
      _payLoad -= tasks.size();
    return tasks;
  }
  bool Processer::store(std::list<Fiber::ptr>& tasks)
  {
    _readyTask.pushk_front(tasks);
    _payLoad += tasks.size();
    return true;
  }

  void Processer::stop()
  {
    _stopping = 1;
    //唤醒epoll_wait
    notify();
  }
  
  //向管道写端写一条空数据
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
    std::vector<struct epoll_event> evs(20);
    //设置线程所在协程为主协程
    setMainFiber(Fiber::getThis());
    //LOG_INFO(sys_log) << "RUN";
    while(1)
      {
	Fiber::ptr fiber;
	while(_readyTask.try_pop_back(fiber))
	  {
	    ASSERT(fiber != nullptr);
	    ASSERT(fiber->getState() == Fiber::READY);
	    fiber->swapIn();
	    ASSERT(fiber->getState() != Fiber::INIT);
	    //加入就绪
	    if(fiber->getState() == Fiber::READY)
	      {
		_readyTask.push_front(std::move(fiber));
	      }
	    else if(fiber->getState() == Fiber::HOLD)
	      {
		--_payLoad;
		//_holdTask.push_front(std::move(fiber));
	      }
	    else if(fiber->getState() == Fiber::TERM
		    || fiber->getState() == Fiber::EXCEPT)
	      {
		--_payLoad;
		fiber.reset();
	      }
	    else
	      {
		ASSERT(false);
	      }
	  }
	//TOD:偷协程
	//LOG_INFO(sys_log) << shared_from_this() ;
	if(_scheduler->balance(shared_from_this()) > 0)
	  {
	    //LOG_INFO(sys_log) << "STOLED";
	    continue;
	  }
	//偷不了则尝试是否可以立即退出
	if(_stopping)
	  {
	    //TODO:改为调度器holdCount
	    if( _readyTask.empty() && _scheduler->getHold() <= 0)
	      {
		//退出run函数
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
	      continue;
	    break;
	  }
	if(rt  < 0 )
	  {
	    LOG_ERROR(sys_log) << "epoll_wait error";
	    continue;
	  }
	char buf[64];
	rt = 0;
	while( (rt = read(_notifyFd[0],buf,64)) )
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
	    if( _readyTask.empty() && _scheduler -> getHold() <=0 )
	      {
		//退出run函数
		return;
	      }
	  }
      }
  }
  
  void Processer::idle()
  {
    LOG_INFO(sys_log) << "Enter Idle Fiber";
    while(!isStopping())
      {
	LOG_DEBUG(sys_log) << " Do Nothing";
	Fiber::YieldToReady();
      }
    LOG_INFO(sys_log) << "Idle Fiber Stopped";
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
