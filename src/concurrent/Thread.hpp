#pragma once

#include <pthread.h>
#include <thread>
#include <memory>
#include <functional>
#include "../latch/latchUtil.hpp"
#include "../latch/lock.hpp"

namespace zhuyh
{
  class Thread : public UnCopyable
  {
  public:
    typedef std::shared_ptr<Thread> ptr;
    Thread(std::function<void()> cb,const std::string& name);
    ~Thread();
    void join();
    const std::string& getThreadName() const
    {
      return m_name;
    }
    pid_t getId() const
    {
      return _id;
    }
    static Thread* getThis();
    static const std::string& thisName();
    static void setName(const std::string& name);
  private:
    static void* run(void* arg);
  private:
    pid_t _id = -1;
    pthread_t m_thread = 0;
    std::function<void()> _cb;
    std::string m_name;
    Semaphore s_sm;
  };
}
