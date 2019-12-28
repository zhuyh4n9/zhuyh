#include "Scheduler.hpp"

namespace zhuyh
{
  static ConfigVar<int>::ptr __max_thread = Config::lookUp<int>("scheduler.maxthread",
							   32,"scheduler max thread");
  static ConfigVar<int>::ptr __min_thread = Config::lookUp<int>("scheduler.minthread",
								32,"scheduler min thread");
  static ConfigVar<int>::ptr __limit_payload = Config::lookUp<int>("scheduler.limitpayload",
							      20,"scheduler limitpayload");
  
  static Logger::ptr sys_log = GET_LOGGER("system");
  
  Scheduler::Scheduler(const std::string& name,int minThread,int maxThread,int limitPayLoad)
    :_minThread(minThread),_maxThread(maxThread),_limitPayLoad(limitPayLoad)
  {
    if(name == "")
      _name = "Scheduler";
    else
      _name = name;
    _pcsQue.resize(_minThread);
  }
  
  //默认构造函数为私有,用于创建根调度器
  Scheduler::Scheduler()
  {
    _minThread = __min_thread->getVar();
    _maxThread = __max_thread->getVar();
    _limitPayLoad = __limit_payload->getVar();
    _pcsQue.resize(_minThread);
    _name = "root";
    
    LOG_DEBUG(sys_log) << "Root Scheduler Created!";
  }
  
  //TODO : to be fixed
  Scheduler::~Scheduler()
  {
    if(!_stopping)
      stop();
    LOG_DEBUG(sys_log) << "Scheduler : "<<_name<<" Destroyed";
  }
  int Scheduler::getHold()
  {
    if(_ioMgr == nullptr) return 0;
    return _ioMgr -> _holdCount;
  }
  //由于进入Main函数之前,根调度器就已经创建并且运行,
  //因此用户单独创建IOManager可行,但是没有意义
  void Scheduler::start()
  {
    ASSERT(_stop == true);
    ASSERT(_minThread > 0 && _maxThread > 0);
    ASSERT(_maxThread >= _minThread);
    ASSERT(_limitPayLoad > 0 );
    //LOG_INFO(sys_log) << "HERE";
    _ioMgr.reset(new IOManager());
    _currentThread = _minThread;
    for(int i=0;i<_minThread;i++)
      {
	std::stringstream ss;
	ss<<_name<<"_processer_"<<i;
	//LOG_INFO(sys_log) << shared_from_this() << std::endl;
	_pcsQue[i].reset(new Processer(ss.str()) );
      }
    for(int i = 0;i<_minThread;i++)
      {
	_pcsQue[i]->start();
      }
    //LOG_DEBUG(sys_log) << "Scheduler Created!";
    _stop = false;
  }
  
  //线程不安全
  void Scheduler::stop()
  {
    ASSERT(_stopping == false);
    _stopping = true;
    //先告诉IO管理器,使其不再接收新的连接
    _ioMgr->stop();
    //LOG_INFO(sys_log) << _currentThread;
    for(int i = 0;i<_currentThread;i++)
      {
	_pcsQue[i]->stop();
	_pcsQue[i]->join();
      }
    _ioMgr->join();
    _currentThread = 0;
    LOG_DEBUG(sys_log)<<"Scheduler Stopped";
    _stop = true;
  }

  void Scheduler::addNewTask(std::shared_ptr<Task> task)
  {
    if(_stopping == true) return;
    addTask(task);
  }
  
  int Scheduler::addReadEvent(int fd,std::shared_ptr<Task> task)
  {
    if(_stopping == true) return -1;
    return  _ioMgr -> addEvent( fd,task,IOManager::READ);
  }
  int Scheduler::addWriteEvent(int fd,std::shared_ptr<Task> task)
  {
    if(_stopping == true) return -1;
    return _ioMgr -> addEvent(fd,task,IOManager::WRITE);
  }

  //优化 &　可能有bug
  int Scheduler::balance(Processer::ptr prc)
  {
    Processer::ptr _prc = getMaxPayLoad();
    //Processer::ptr _prc = _pcsQue[rand()%_currentThread];
    //LOG_INFO(sys_log) << "HERE";
    //同一个线程
    if(_prc->_thread.get() == Thread::getThis() ) return 0;
    std::list<Fiber::ptr> fibers;
    //偷取协程
    fibers = _prc->steal(_prc->_payLoad / 2);
    int sz = fibers.size();
    prc->store(fibers);
    return sz;
  }

  //根据负载创建新执行器
  void Scheduler::addTask(std::shared_ptr<Task> task)
  {
    Processer::ptr p = getMinPayLoad();
    //Processer::ptr p = _pcsQue[rand()%_currentThread];
    p->addTask(task);
  }
  void Scheduler::addTask(CbType cb)
  {
    addTask(std::shared_ptr<Task>(new Task(cb)));
  }
  void Scheduler::addTask(Fiber::ptr fiber)
  {
    addTask(std::shared_ptr<Task>(new Task(fiber)));
  }

  Scheduler* Scheduler::getThis()
  {
    static Scheduler::ptr _scheduler(new Scheduler());
    return _scheduler.get();
  }

  //可能会有bug
  Processer::ptr Scheduler::getMaxPayLoad()
  {
    Processer::ptr _prc = nullptr;
    for(unsigned i=0;i < _pcsQue.size() ;i++)
      {
	//LOG_INFO(sys_log) << "HERE";
	if(_prc == nullptr)
	  _prc = _pcsQue[i];
	else
	  _prc = _prc->_payLoad > _pcsQue[i]->_payLoad ? _prc : _pcsQue[i];
      }
    return _prc;
  }

  //可能会有bug
  Processer::ptr Scheduler::getMinPayLoad()
  {
    Processer::ptr _prc = nullptr;
    for(unsigned i=0;i < _pcsQue.size() ;i++)
      {
	if(_prc == nullptr)
	  _prc = _pcsQue[i];
	else
	  _prc = _prc->_payLoad < _pcsQue[i]->_payLoad ? _prc : _pcsQue[i];
      }
    return _prc;
  }
  int Scheduler::addTimer(Timer::ptr timer,std::function<void()> cb,
		 Timer::TimerType type )
   {
     if(_ioMgr != nullptr)
       return _ioMgr->addTimer(timer,cb,type);
     return -1;
   }
  int Scheduler:: addTimer(Timer::ptr* timer,std::function<void()> cb,
		Timer::TimerType type )
  {
    if(_ioMgr != nullptr)
      return _ioMgr->addTimer(timer,cb,type);
    return -1;
  }
}
