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
  Processer::Processer(const std::string name,Scheduler* schd)
    :_name(name)
  {
    //根调度器
    if(schd == nullptr)
      _scheduler = Scheduler::Schd::getInstance();
    else
      _scheduler = schd;
  }

  Processer::~Processer()
  {
    if(!_stopping)
      stop();
    //LOG_INFO(sys_log) << "Processer : "<<_name<<"  Destroyed, worked = :"<<worked;
  }
  
  void Processer::start(CbType cb)
  {
    if(cb == nullptr)
      {
	try
	  {
	    _thread.reset(new Thread(std::bind(&Processer::run,shared_from_this()),_name));
	  }
	catch (std::exception& e)
	  {
	    LOG_ERROR(sys_log) << e.what();
	  }
      }
    else
      {
	Thread::setName(_name);
	addTask(Task::ptr(new Task(cb)));
	run();
      }
	//LOG_INFO(sys_log) << "Processer Started!";
  }

  bool Processer::addTask(Task::ptr task)
  {
    //LOG_ROOT_ERROR() << "ADD NEW TASK1";
    ASSERT(_scheduler != nullptr);
    if(task == nullptr) return false;
    if(task->cb || task -> fiber)
      {
	if(task->cb)
	  {
	    ASSERT(task -> fiber == nullptr);
	    ++(_scheduler->totalTask);
	  }
	ASSERT(task != nullptr);
	_readyTask.push_front(task);
	++_payLoad;
	notify();
	//LOG_ROOT_ERROR() << "ADD NEW TASK2";
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
    try{
      m_sem.notify();
    } catch(std::exception& e){
      LOG_WARN(sys_log)<<"notify failed : "<<e.what();
      return -1;
    }
    return 0;
  }

  int Processer::waitForNotify(){
    try{
      m_sem.wait();
    }catch(std::exception& e){
      LOG_WARN(sys_log)<<"waitForNotify failed : "<<e.what();
      return -1;
    }
    return 0;
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
    Scheduler::setThis(_scheduler);
    Hook::setHookState(true);
    setMainFiber(Fiber::getThis());
    //ASSERT(_readyTask.empty());
    while(1)
      {
	Task::ptr task;
	//LOG_INFO(sys_log) << " totalTask : "<<_readyTask.size();
	while(_readyTask.try_pop_back(task))
	  {
	    //LOG_ROOT_ERROR() << "Get New Task";
	    ASSERT(task != nullptr);
	    worked++;
	    if(task->fiber)
	      {
		if(task -> fiber->getState() != Fiber::HOLD &&
		   task -> fiber->getState() != Fiber::READY)
		  // ASSERT2(task -> fiber->getState() == Fiber::EXEC,
		  // 	  Fiber::getState(task->fiber->getState()));
		  // 说明未完全切换HOLD态
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

	    //LOG_ROOT_ERROR() << "Swapped Out";
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
		//LOG_INFO(sys_log) << "EXIT PROCESSOR1";
		return;
	      }
	  }
	waitForNotify();
	if(_stopping)
	  {
	    //TODO:改为调度器holdCount
	    if(_scheduler->totalTask <= 0
	       && _readyTask.empty()
	       && _scheduler->getHold() <= 0)
	      {
		//LOG_INFO(sys_log) << "EXIT PROCESSOR";
		--_scheduler->m_currentThread;
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
