#include "Thread.hpp"
#include "../logUtil.hpp"
#include "../macro.hpp"

namespace zhuyh
{
  //系统调用日志
  static Logger::ptr sys_log = GET_LOGGER("system");
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
	__thisThread->_name = name;
      }
    __thisName = name;
  }
  
  Thread::Thread(std::function<void()> cb,const std::string& name)
    :_cb(cb),_name(name)
  {
    if(name.empty() )
      {
	_name = "UNKNOWN";
      }
    if(name.size() > 20 )
      {
	_name = name.substr(0,19);
	LOG_INFO(sys_log) << "thread name " << name
			  << " is longer than 16 character..."
			  << " current thread name : "<<_name;
      }
    int rt = pthread_create(&_thread,nullptr,&Thread::run,(void*)this);
    if(rt)
      {
	LOG_ERROR(sys_log) << "Failed to create thread : "<<_name;
	throw std::logic_error("pthread_create error");
      }
    //确保线程按照创建顺序启动
    s_sm.wait();
  }

  Thread::~Thread()
  {
    if(_thread)
      {
	pthread_detach(_thread);
	_thread = 0;
      }
  }

  void Thread::join()
  {
    if(_thread)
      {
	pthread_join(_thread,nullptr);
	_thread = 0;
      }
  }

  void* Thread::run(void* arg)
  {
    Thread* thread = (Thread*)arg;
    __thisThread =  thread;
    __thisName = thread->_name;
    thread->_id = getThreadId();
    //设置线程名
    pthread_setname_np(pthread_self(),thread->_name.c_str());
    //信号量加1,表示线程已经成功创建,可以进行创建其他线程了
    std::function<void()> cb;
    cb.swap(thread->_cb);
    thread->s_sm.notify();
    cb();
    return nullptr;
  }
  
}
