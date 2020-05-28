#include "Scheduler.hpp"
#include "../config.hpp"
#include "../macro.hpp"
#include "../logUtil.hpp"
#include "IOManager.hpp"
#include "Processer.hpp"

namespace zhuyh
{
  static ConfigVar<int>::ptr __max_thread = Config::lookUp<int>("scheduler.maxthread",
							   64,"scheduler max thread");
  static ConfigVar<int>::ptr __min_thread = Config::lookUp<int>("scheduler.minthread",
								32,"scheduler min thread");
  
  static Logger::ptr sys_log = GET_LOGGER("system");

  static thread_local Scheduler* s_schd;
  Scheduler* Scheduler::getThis()
  {
    return s_schd;
  }
  void Scheduler::setThis(Scheduler* schd)
  {
    s_schd = schd;
  }
  Scheduler::Scheduler(const std::string& name,int threads)
    :_minThread(threads),_maxThread(threads)
  {
    if(name == "")
      _name = "root";
    else
      _name = name;
    m_lgThread = SingletonPtr<LogThread>::getInstance();
    // std::cout<<"use Count : "<<m_lgThread.use_count()<<" use count2 :" 
    // 	     <<SingletonPtr<LogThread>::getInstance().use_count()<<std::endl;
    _pcsQue.resize(_minThread);
  }
  
  //默认构造函数为私有,用于创建根调度器
  Scheduler::Scheduler()
    :totalTask{0}
  {
    _minThread = __min_thread->getVar();
    _maxThread = __max_thread->getVar();
    _pcsQue.resize(_minThread);
    m_lgThread = SingletonPtr<LogThread>::getInstance();
    _name = "root";
    LOG_DEBUG(sys_log) << "Root Scheduler Created!";
  }
  
  //TODO : to be fixed
  Scheduler::~Scheduler()
  {
    //std::cout<<"Call ~Scheduler1\n"<<Bt2Str(100,0,"   ");
    if(_stopping == false)
      stop();
    //std::cout<<"Call ~Scheduler2\n"<<Bt2Str(100,0,"   ");
    //LOG_INFO(sys_log) << "Scheduler : "<<_name<<" Destroyed";
  }
  int Scheduler::getHold()
  {
    if(_ioMgr == nullptr) return 0;
    return _ioMgr -> _holdCount;
  }
  
  void Scheduler::start(CbType cb)
  {
    if(_stop == false) return;
    ASSERT(_minThread > 0 && _maxThread > 0);
    ASSERT(_maxThread >= _minThread);
    setThis(this);
    
    _ioMgr.reset(new IOManager(_name+"_schd",getThis()));
    m_currentThread = _minThread;
    for(int i=0;i<_minThread;i++){
	std::stringstream ss;
	ss<<_name<<"_processer_"<<i;
	_pcsQue[i].reset(new Processer(ss.str(),getThis()));
      }
    for(int i = 0;i<_minThread-1;i++){
	_pcsQue[i]->start();
      }
    if(cb){
      _pcsQue[_minThread-1]->start(cb);
    }else{
      _pcsQue[_minThread-1]->start();
    }
    m_dispatcher.reset(new Thread(std::bind(&Scheduler::dispatcher,this),_name+"_notifier"));
    
    LOG_DEBUG(sys_log) << "Scheduler Created!";
    _stop = false;
  }
  
  //线程不安全
  void Scheduler::stop()
  {
    if(_stopping == true ) return;
    ASSERT(_stopping == false);
    _stopping = true;
    _ioMgr->stop();
    //LOG_INFO(sys_log) << m_currentThread;
    for(size_t i = 0;i<_pcsQue.size();i++)
      {
	_pcsQue[i]->stop();
	_pcsQue[i]->join();
      }
    _ioMgr->join();
    m_dispatcher->join();
    _stop = true;
  }
  
  void Scheduler::addNewTask(std::shared_ptr<Task> task)
  {
    ASSERT2(_stop == false,"Scheduler should start first");
    addTask(task);
  }

  void Scheduler::addNewTask(CbType cb)
  {
    ASSERT2(_stop == false,"Scheduler should start first");
    Task::ptr task(new Task(cb));
    addTask(task);
  }

  void Scheduler::addNewTask(Fiber::ptr fiber)
  {
    ASSERT2(_stop == false,"Scheduler should start first");
    Task::ptr task(new Task(fiber));
    addTask(task);
  }
  
  int Scheduler::addReadEvent(int fd,std::function<void()> cb)
  {
    //(_stopping == true) ASSERT(0);
    Task::ptr task = nullptr;
    if(cb == nullptr)
      task.reset(new Task(Fiber::getThis()));
    else
      task.reset(new Task(cb));
    return _ioMgr -> addEvent(fd,task,IOManager::READ);
  }
  
  int Scheduler::addWriteEvent(int fd,std::function<void()> cb)
  {
    //(_stopping == true) ASSERT(false);
    Task::ptr task = nullptr;
    if(cb == nullptr)
      task.reset(new Task(Fiber::getThis()));
    else
      task.reset(new Task(cb));
    return _ioMgr -> addEvent(fd,task,IOManager::WRITE);
  }

  //优化 &　可能有bug
  int Scheduler::balance(Processer::ptr prc)
  {
    if(_minThread == 1) return 0;
    Processer::ptr _prc = getMaxPayLoad();
    if(_prc->_thread.get() == Thread::getThis() ) return 0;
    std::list<Task::ptr> tasks;
    //偷取协程
    tasks = _prc->steal(_prc->_payLoad / 2);
    int sz = tasks.size();
    prc->store(tasks);
    return sz;
  }
  void Scheduler::addHold()
  {
    ++(_ioMgr->_holdCount);
  }
  void Scheduler::delHold()
  {
    --(_ioMgr->_holdCount);
  }
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
  void Scheduler::addTask(std::shared_ptr<Task>* task)
  {
    Task::ptr t = nullptr;
    t.swap(*task);
    addTask(t);
  }
  // Scheduler* Scheduler::getThis()
  // {
  //   static Scheduler::ptr _scheduler(new Scheduler());
  //   return _scheduler.get();
  // }

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
	if(_pcsQue[i]->_payLoad == 0) return _pcsQue[i];
	if(_prc == nullptr)
	  _prc = _pcsQue[i];
	else
	  _prc = _prc->_payLoad < _pcsQue[i]->_payLoad ? _prc : _pcsQue[i];
      }
    return _prc;
  }
  
  int Scheduler::addTimer(Timer::ptr timer,std::function<void()> cb,
			  Timer::TimerType type)			
  {
    if(_ioMgr != nullptr)
      {
	int rt = 0;
	if(cb == nullptr)
	  {
	    //主协程只能添加回调函数
	    if(Fiber::getThis() == Fiber::getMain()) return -1;
	    rt = _ioMgr->addTimer(timer,cb,type);			
	    if(rt >= 0)
	      {
		Fiber::YieldToHold();
	      }
	    ASSERT(rt >=0);
	    return rt;
	  }
	else
	  {
	    rt = _ioMgr->addTimer(timer,cb,type);
	    ASSERT(rt >= 0);
	    return rt;
	  }
      }								       
    return -1;								
  }
  int Scheduler::cancleReadEvent(int fd)
  {
    return _ioMgr->cancleEvent(fd,IOManager::READ);
  }
  
  int Scheduler::cancleWriteEvent(int fd)
  {
    return _ioMgr->cancleEvent(fd,IOManager::WRITE);
  }

  int Scheduler::cancleAllEvent(int fd)
  {
    return _ioMgr->cancleAll(fd);
  }

  void Scheduler::dispatcher(){
    while(m_currentThread > 0){
      for(auto& item : _pcsQue){
	item->notify();
      }
      //LOG_WARN(sys_log) << "left : "<<m_currentThread;
      usleep(100*1000);
    }
  }
}
