#pragma once

#include "log.hpp"
#include <thread>
#include "scheduler/TSQueue.hpp"
#include "Singleton.hpp"
#include <memory>
#include "latch/lock.hpp"

namespace zhuyh
{
  class LogThread final
  {
  public:
    typedef std::shared_ptr<LogThread> ptr;
    friend class SingletonPtr<LogThread>;
  private:
    LogThread();
    //not allowed
    LogThread(const LogThread& o) = delete;
    LogThread& operator=(const LogThread& o) = delete;
    void run();
    void work();
  public:
    //开启线程日志
    void start();
    //关闭
    void close();
    //发送日志
    void sendLog(LogEvent::ptr event);
    
    ~LogThread();
  private:
    NonbTSQueue<LogEvent::ptr,Mutex> m_que;
    int m_notifyFd[2];
    bool m_close;
    std::thread m_thread;
    int m_epfd;
  };
}
