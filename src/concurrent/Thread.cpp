#include "Thread.hpp"
#include "../logUtil.hpp"
#include "../macro.hpp"

namespace zhuyh
{
  //系统调用日志
  static Logger::ptr s_syslog = GET_LOGGER("system");
  static thread_local Thread* __thisThread = nullptr;
  static thread_local std::string __thisName = "NONE" ;
  
  Thread* Thread::getThis()
  {
    return __thisThread;
  }

  const std::string& Thread::thisName()
  {
    return __thisName;
  }
  
  void Thread::setName(const std::string& name)
  {
    if(__thisThread)
      {
	__thisThread->m_name = name;
      }
    __thisName = name;
  }
  
  Thread::Thread(std::function<void()> cb,const std::string& name)
    :_cb(cb),m_name(name)
  {
    if(name.empty() )
      {
	m_name = "UNKNOWN";
      }
    if(name.size() > 20 )
      {
	m_name = name.substr(0,19);
	LOG_INFO(s_syslog) << "thread name " << name
			  << " is longer than 16 character..."
			  << " current thread name : "<<m_name;
      }
    int rt = pthread_create(&m_thread,nullptr,&Thread::run,(void*)this);
    if(rt)
      {
	LOG_ERROR(s_syslog) << "Failed to create thread : "<<m_name;
	throw std::logic_error("pthread_create error");
      }
    //确保线程按照创建顺序启动
    s_sm.wait();
  }

  Thread::~Thread()
  {
    if(m_thread)
      {
	pthread_detach(m_thread);
	m_thread = 0;
      }
  }

  void Thread::join()
  {
    if(m_thread)
      {
	pthread_join(m_thread,nullptr);
	m_thread = 0;
      }
  }

  void* Thread::run(void* arg)
  {
    Thread* thread = (Thread*)arg;
    __thisThread =  thread;
    __thisName = thread->m_name;
    thread->_id = getThreadId();
    //设置线程名
    pthread_setname_np(pthread_self(),thread->m_name.c_str());
    //信号量加1,表示线程已经成功创建,可以进行创建其他线程了
    std::function<void()> cb;
    cb.swap(thread->_cb);
    thread->s_sm.notify();
    cb();
    return nullptr;
  }
  
}
