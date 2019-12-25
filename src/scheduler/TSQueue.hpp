#pragma once

#include <deque>
#include "../latch/lock.hpp"
#include <list>

namespace zhuyh
{
  //支持批量加入的线程安全队列
  template<class Type>
  class TSQueue
  {
  public:
    TSQueue(int limit = -1)
      :_limit(limit)
    {}
    //非阻塞方法
    //批量弹出
    bool try_popk_front(int k,std::list<Type>& c)
    {
      LockGuard lg(mx);
      if(deq.empty()) return false;
      for(int i=0;i<k;i++)
	{
	  if(deq.empty()) break;
	  c.push_back(deq.front());
	  deq.pop_front();
	}
      return true;
    }
    bool try_popk_back(int k,std::list<Type>& c)
    {
      LockGuard lg(mx);
      if(deq.empty() ) return false;
      for(int i=0;i<k;i++)
	{
	  if(deq.empty()) break;
	  c.push_back(deq.back());
	  deq.pop_back();
	}
      return true;
    }
    //单个弹出
    bool try_pop_back(Type& v)
    {
      LockGuard lg(mx);
      if(deq.empty()) return false;
      v = deq.back();
      deq.pop_back();
      return true;
    }
    bool try_pop_front(Type& v)
    {
      LockGuard lg(mx);
      if(deq.empty()) return false;
      v = deq.front();
      deq.pop_front();
      return true;
    }
    
    //阻塞方法
    void pop_front(Type& v)
    {
      LockGuard lg(mx);
      while(deq.empty())
	{
	  lg.unlock();
	  nput.wait();
	  lg.lock();
	}
      v=deq.front();
      deq.pop_front();
    }

    void pop_back(Type& v)
    {
      LockGuard lg(mx);
      while(deq.empty())
	{
	  lg.unlock();
	  nput.wait();
	  lg.lock();
	}
      v = deq.back();
      deq.pop_back();
    }
    int popk_front(int k,std::list<Type>& c)
    {
      int real = 0;
      LockGuard lg(mx);
      while(deq.empty())
	{
	  lg.unlock();
	  nput.wait();
	  lg.lock();
	}
      for(int i=0;i<k;i++)
	{
	  if(deq.empty()) break;
	  c.push_back(deq.front());
	  deq.pop_front();
	  real++;
	}
      return real;
    }
    int popk_back(int k,std::list<Type>& c)
    {
      int real = 0;
      LockGuard lg(mx);
      while(deq.empty())
	{
	  lg.unlock();
	  nput.wait();
	  lg.lock();
	}
      for(int i=0;i<k;i++)
	{
	  if(deq.empty()) break;
	  c.push_back(deq.back());
	  deq.pop_back();
	  real++;
	}
      return real;
    }
    
    //批量压入
    void pushk_front(std::list<Type>& c)
    {
      LockGuard lg(mx);
      for(auto& v:c)
	{
	  deq.push_front(v);
	  //nput.notify();
	}
      c.clear();
      nput.notify();
    }
    void pushk_back(std::list<Type>& c)
    {
      LockGuard lg(mx);
      for(auto& v:c)
	{
	  deq.push_back(v);
	  //nput.notify();
	}
      c.clear();
      nput.notify();
    }

    //单个压入
    void push_back(Type&& v)
    {
      LockGuard lg(mx);
      if(_limit != -1 && (int)deq.size() >= _limit ) return;
      deq.push_back(v);
      nput.notify();
    }
    void push_front(Type&& v)
    {
      LockGuard lg(mx);
      if(_limit != -1 && (int)deq.size() >= _limit ) return;
      deq.push_front(v);
      nput.notify();
    }

    bool empty() const
    {
      LockGuard lg(mx);
      return deq.empty();
    }
    void clear()
    {
      LockGuard lg(mx);
      deq.clear();
    }
    void setLimit(int limit)
    {
      LockGuard lg(mx);
      _limit = limit;
    }
  private:
    std::deque<Type> deq;
    mutable Semaphore nput;
    mutable SpinLock mx;
    int _limit = -1;
  };
  
}
