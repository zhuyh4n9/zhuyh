#pragma once

#include <iostream>
#include "latchUtil.hpp"
#include <semaphore.h>
#include <atomic>
#include <thread>
#include <pthread.h>

namespace zhuyh
{
  class Semaphore : public UnCopyable
  {
  public:
    Semaphore(uint32_t count = 0)
    {
    if(sem_init(&m_sem,0,count))
      {
	throw std::logic_error("sem_init error");
      }
    }
    
    ~Semaphore()
    {
      sem_destroy(&m_sem);
    }
    
    void wait()
    {
      if(sem_wait(&m_sem))
	{
	  throw std::logic_error("sem_wait error");
	}
    }
    
    void notify()
    {
      if(sem_post(&m_sem))
	{
	  throw std::logic_error("sem_post error");
	}
    }
    
  private:
    sem_t m_sem;
  };
  //虚基类
  class ILock : public UnCopyable
  {
  public:
    virtual void lock() = 0;
    virtual void unlock() = 0;
    virtual ~ILock() {};
    ILock() {}
  };

  class IRWLock : public UnCopyable
  {
  public:
    virtual void rdLock() = 0;
    virtual void wrLock() = 0;
    virtual void unlock() = 0;

    virtual ~IRWLock() {};
    IRWLock() {}
  };
  
  //自旋锁
  class SpinLock : public ILock
  {
  public:
    //tas实现自旋锁
    void lock() override
    {
      pthread_spin_lock(&lk);
    }

      //清除flag标记
    void unlock() override
    {
      pthread_spin_unlock(&lk);
    }
      
    SpinLock()
    {
      pthread_spin_init(&lk,0);
    }
    ~SpinLock()
    {
      pthread_spin_destroy(&lk);
    }
  private:
    pthread_spinlock_t lk;
  };
  
  //互斥锁
  class Mutex : public ILock
  {
  public:
    Mutex()
      :s_mx(1)
    {
    }
    ~Mutex() {}
    void lock()
    {
      s_mx.wait();
    }
    void unlock()
    {
      s_mx.notify();
    }
  private:
    Semaphore s_mx;
  };

  class IdleLock : public ILock
  {
    void lock() {}
    void unlock() {}
  };
  class RWLock : public IRWLock
  {
  public:
    RWLock()
    {
      if(pthread_rwlock_init(&_rwlk,nullptr) )
	{
	  throw std::logic_error("pthread_rwlock_init error");
	}
    }
    
    ~RWLock()
    {
      pthread_rwlock_destroy(&_rwlk);
    }
    
    void rdLock() override
    {
      pthread_rwlock_rdlock(&_rwlk);
    }
    
    void wrLock() override
    {
      pthread_rwlock_wrlock(&_rwlk);
    }
    void unlock() override
    {
      pthread_rwlock_unlock(&_rwlk);
    }
  private:
    pthread_rwlock_t _rwlk;
  };
  
  //TODO:去出调试选项
#define XX(Name,LockBase,lockName1)			\
  Name(LockBase& lk)					\
    :m_lk(lk)						\
  {							\
    lockName1();					\
  }							\
  ~Name()						\
  {							\
    if(isLocked) unlock();				\
  }							\
  void lockName1()					\
  {							\
    isLocked = true;					\
    m_lk.lockName1();					\
  }							\
  void unlock()						\
  {							\
    if(isLocked == true)				\
      {							\
	isLocked = false;				\
	m_lk.unlock();					\
      }							\
  }

  class LockGuard : private UnCopyable
  {
  public:
    XX(LockGuard,ILock,lock);
  private:
    bool isLocked = false;
    ILock& m_lk;
  };
  
  class RDLockGuard : private UnCopyable
  {
  public:
    XX(RDLockGuard,IRWLock,rdLock);
  private:
    bool isLocked = false;
    IRWLock& m_lk;
  };
  class WRLockGuard : private UnCopyable
  {
  public:
    XX(WRLockGuard,IRWLock,wrLock);
  private:
    bool isLocked = false;
    IRWLock& m_lk;
  };
#undef XX
  
}

