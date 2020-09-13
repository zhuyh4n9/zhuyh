#include "Scheduler.hpp"
#include "config.hpp"
#include "macro.hpp"
#include "logUtil.hpp"
#include "Reactor.hpp"
#include "Processer.hpp"

namespace zhuyh {
static ConfigVar<int>::ptr s_maxThreadCfg = Config::lookUp<int>("scheduler.maxthread",
							   64,"scheduler max thread");
static ConfigVar<int>::ptr s_minThreadCfg = Config::lookUp<int>("scheduler.minthread",
								32,"scheduler min thread");
static Logger::ptr s_syslog = GET_LOGGER("system");
static thread_local Scheduler* s_sched;

Scheduler* Scheduler::getThis() {
    return s_sched;
}

void Scheduler::setThis(Scheduler* sched) {
    s_sched = sched;
}
Scheduler::Scheduler(const std::string& name,int threads)
    :m_minThread(threads),m_maxThread(threads) {
    if(name == "")
        m_name = "root";
    else
        m_name = name;
    m_lgThread = SingletonPtr<LogThread>::getInstance();
    // std::cout<<"use Count : "<<m_lgThread.use_count()<<" use count2 :" 
    // 	     <<SingletonPtr<LogThread>::getInstance().use_count()<<std::endl;
    m_pcsQue.resize(m_minThread);
}
  
  //默认构造函数为私有,用于创建根调度器
Scheduler::Scheduler()
    :m_totalTask{0} {
    m_minThread = s_minThreadCfg->getVar();
    m_maxThread = s_maxThreadCfg->getVar();
    m_pcsQue.resize(m_minThread);
    m_lgThread = SingletonPtr<LogThread>::getInstance();
    m_name = "root";
    LOG_DEBUG(s_syslog) << "Root Scheduler Created!";
  }
  
  //TODO : to be fixed
Scheduler::~Scheduler() {
    //std::cout<<"Call ~Scheduler1\n"<<Bt2Str(100,0,"   ");
    if(m_stopping == false)
      stop();
    //std::cout<<"Call ~Scheduler2\n"<<Bt2Str(100,0,"   ");
    //LOG_INFO(s_syslog) << "Scheduler : "<<m_name<<" Destroyed";
}

int Scheduler::getHold() {
    if(m_reactor == nullptr) return 0;
    return m_reactor -> m_holdCount;
}
  
void Scheduler::start(CbType cb) {
    if(m_stop == false) return;

    ASSERT(m_minThread > 0 && m_maxThread > 0);
    ASSERT(m_maxThread >= m_minThread);
    setThis(this);
    
    m_reactor.reset(new Reactor(m_name+"_schd",getThis()));
    m_currentThread = m_minThread;
    for(int i=0; i < m_minThread; i++){
        std::stringstream ss;
        ss<<m_name<<"_processer_"<<i;
        m_pcsQue[i].reset(new Processer(ss.str(),getThis()));
    }
    for(int i = 0;i<m_minThread-1;i++){
	    m_pcsQue[i]->start();
    }
    if(cb){
        m_pcsQue[m_minThread-1]->start(cb);
    } else {
        m_pcsQue[m_minThread-1]->start();
    }
    m_dispatcher.reset(new Thread(std::bind(&Scheduler::dispatcher,this), m_name+"_notifier"));
    
    LOG_DEBUG(s_syslog) << "Scheduler Created!";
    m_stop = false;
}
  
  //线程不安全
void Scheduler::stop() {
    if (m_stopping == true )
            return;
    ASSERT(m_stopping == false);
    m_stopping = true;
    m_reactor->stop();
    //LOG_INFO(s_syslog) << m_currentThread;
    for (size_t i = 0; i < m_pcsQue.size(); i++) {
        m_pcsQue[i]->stop();
        m_pcsQue[i]->join();
    }
    m_reactor->join();
    m_dispatcher->join();
    m_stop = true;
}
  
void Scheduler::addNewTask(std::shared_ptr<Task> task){
    ASSERT2(m_stop == false, "Scheduler should start first");
    addTask(task);
}

void Scheduler::addNewTask(CbType cb) {
    ASSERT2(m_stop == false, "Scheduler should start first");
    Task::ptr task(new Task(cb));
    addTask(task);
}

void Scheduler::addNewTask(Fiber::ptr fiber) {
    ASSERT2(m_stop == false,"Scheduler should start first");
    Task::ptr task(new Task(fiber));
    addTask(task);
}
  
int Scheduler::addReadEvent(int fd,std::function<void()> cb) {
    //(m_stopping == true) ASSERT(0);
    Task::ptr task = nullptr;
    if(cb == nullptr)
      task.reset(new Task(Fiber::getThis()));
    else
      task.reset(new Task(cb));
    return m_reactor -> addEvent(fd,task,Reactor::READ);
}

int Scheduler::addWriteEvent(int fd,std::function<void()> cb) {
    //(m_stopping == true) ASSERT(false);
    Task::ptr task = nullptr;
    if(cb == nullptr)
      task.reset(new Task(Fiber::getThis()));
    else
      task.reset(new Task(cb));
    return m_reactor -> addEvent(fd,task,Reactor::WRITE);
}

  //优化 &　可能有bug
int Scheduler::balance(Processer::ptr prc) {
    if (m_minThread == 1)
        return 0;
    Processer::ptr _prc = getMaxPayLoad();
    if (_prc->m_thread.get() == Thread::getThis()) 
        return 0;
    std::list<Task::ptr> tasks;
    //偷取协程
    tasks = _prc->steal(_prc->m_payLoad / 2);
    int sz = tasks.size();
    prc->store(tasks);
    return sz;
}

void Scheduler::addHold() {
    ++(m_reactor->m_holdCount);
}

void Scheduler::delHold(){
    --(m_reactor->m_holdCount);
}

void Scheduler::addTask(std::shared_ptr<Task> task){
    Processer::ptr p = getMinPayLoad();
    //Processer::ptr p = _pcsQue[rand()%_currentThread];
    p->addTask(task);
}

void Scheduler::addTask(CbType cb) {
    addTask(std::shared_ptr<Task>(new Task(cb)));
}

void Scheduler::addTask(Fiber::ptr fiber) {
    addTask(std::shared_ptr<Task>(new Task(fiber)));
}

void Scheduler::addTask(std::shared_ptr<Task>* task) {
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
Processer::ptr Scheduler::getMaxPayLoad() {
    Processer::ptr prc = nullptr;
    for (unsigned i=0; i < m_pcsQue.size() ;i++) {
	//LOG_INFO(s_syslog) << "HERE";
        if (prc == nullptr) {
            prc = m_pcsQue[i];
        } else {
            prc = (prc->m_payLoad > m_pcsQue[i]->m_payLoad) ? prc : m_pcsQue[i];
        }
    }
    return prc;
}

  //可能会有bug
Processer::ptr Scheduler::getMinPayLoad() {
    Processer::ptr prc = nullptr;
    for (unsigned i=0; i < m_pcsQue.size(); i++) {
        if(m_pcsQue[i]->m_payLoad == 0) 
            return m_pcsQue[i];
        if(prc == nullptr) {
            prc = m_pcsQue[i];
        } else {
            prc = (prc->m_payLoad < m_pcsQue[i]->m_payLoad) ? prc : m_pcsQue[i];
        }
    }

    return prc;
}
  
int Scheduler::addTimer(Timer::ptr timer,std::function<void()> cb,
			  Timer::TimerType type) {
    if(m_reactor != nullptr) {
        int rt = 0;
        if(cb == nullptr) {
            //主协程只能添加回调函数
            if(Fiber::getThis() == Fiber::getMain())
                return -1;
            rt = m_reactor->addTimer(timer,cb,type);			
            if(rt >= 0) {
                Fiber::YieldToHold();
            }
            ASSERT(rt >=0);
            return rt;
        } else {
            rt = m_reactor->addTimer(timer,cb,type);
            ASSERT(rt >= 0);
            return rt;
        }
    }
    return -1;							
}

int Scheduler::cancelReadEvent(int fd) {
    return m_reactor->cancelEvent(fd,Reactor::READ);
}
  
int Scheduler::cancelWriteEvent(int fd) {
    return m_reactor->cancelEvent(fd,Reactor::WRITE);
}

int Scheduler::cancelAllEvent(int fd){
    return m_reactor->cancelAll(fd);
}

void Scheduler::dispatcher() {
    while(m_currentThread > 0){
        for(auto& item : m_pcsQue){
            item->notify();
        }
        //LOG_WARN(s_syslog) << "left : "<<m_currentThread;
        usleep(100*1000);
    }
}

} // end of namespace zhuyh
