#pragma once

#include <deque>
#include "latch/lock.hpp"
#include <list>

namespace zhuyh
{
  //支持批量加入的线程安全队列
template<class Type>
class TSQueue
{
public:
    TSQueue(int limit = -1)
      :m_limit(limit){
    }
    //非阻塞方法
    //批量弹出
    bool try_popk_front(int k,std::list<Type>& c) {
        LockGuard lg(m_mx);
        if(m_deq.empty()) return false;
        for(int i=0;i<k;i++){
            if(m_deq.empty()) break;
            c.push_back(m_deq.front());
            m_deq.pop_front();
        }
      return true;
    }

    bool try_popk_back(int k,std::list<Type>& c) {
        LockGuard lg(m_mx);
        if (m_deq.empty()) {
            return false;
        }
        for(int i=0;i<k;i++) {
            if(m_deq.empty()) break;
                c.push_back(m_deq.back());
                m_deq.pop_back();
            }
        return true;
    }

    //单个弹出
    bool try_pop_back(Type& v) {
        LockGuard lg(m_mx);
        if(m_deq.empty()) {
            return false;
        }
        v = m_deq.back();
        m_deq.pop_back();
        return true;
    }

    bool try_pop_front(Type& v)
    {
      LockGuard lg(m_mx);
      if (m_deq.empty()) {
          return false;
      }
      v = m_deq.front();
      m_deq.pop_front();
      return true;
    }

    //阻塞方法
    void pop_front(Type& v) {
        LockGuard lg(m_mx);
        while (m_deq.empty()) {
            lg.unlock();
            m_nput.wait();
            lg.lock();
        }
        v=m_deq.front();
        m_deq.pop_front();
    }

    void pop_back(Type& v) {
        LockGuard lg(m_mx);
        while (m_deq.empty()) {
            lg.unlock();
            m_nput.wait();
            lg.lock();
        }
        v = m_deq.back();
        m_deq.pop_back();
    }

    int popk_front(int k,std::list<Type>& c) {
        int real = 0;
        LockGuard lg(m_mx);
        while (m_deq.empty()) {
            lg.unlock();
            m_nput.wait();
            lg.lock();
        }
        for(int i=0;i<k;i++) {
            if(m_deq.empty()) break;
            c.push_back(m_deq.front());
            m_deq.pop_front();
            real++;
        }
        return real;
    }

    int popk_back(int k,std::list<Type>& c)
    {
        int real = 0;
        LockGuard lg(m_mx);
        while (m_deq.empty()) {
            lg.unlock();
            m_nput.wait();
            lg.lock();
        }
        for(int i=0;i<k;i++) {
        if(m_deq.empty()) break;
            c.push_back(m_deq.back());
            m_deq.pop_back();
            real++;
        }
        return real;
    }

    //批量压入
    void pushk_front(std::list<Type>& c) {
        LockGuard lg(m_mx);
        for(auto& v:c) {
            m_deq.push_front(v);
            //m_nput.notify();
        }
        c.clear();
        m_nput.notify();
    }

    void pushk_back(std::list<Type>& c) {
        LockGuard lg(m_mx);
        for (auto& v:c) {
            m_deq.push_back(v);
            //m_nput.notify();
        }
        c.clear();
        m_nput.notify();
    }

    //单个压入
    void push_back(const Type& v) {
        LockGuard lg(m_mx);
        //if(m_limit != -1 && (int)deq.size() >= m_limit ) return;
        m_deq.push_back(v);
        m_nput.notify();
    }

    void push_front(const Type& v) {
        LockGuard lg(m_mx);
        //if(m_limit != -1 && (int)deq.size() >= m_limit ) return;
        m_deq.push_front(v);
        m_nput.notify();
    }

    bool empty() const {
        LockGuard lg(m_mx);
        return m_deq.empty();
    }

    void clear() {
        LockGuard lg(m_mx);
        m_deq.clear();
    }

    void setLimit(int limit) {
        LockGuard lg(m_mx);
        m_limit = limit;
    }
private:
    std::deque<Type> m_deq;
    mutable Semaphore m_nput;
    mutable SpinLock m_mx;
    int m_limit = -1;
};
  
template<class Type,class MutexType = SpinLock>
class NonbTSQueue {
public:
    NonbTSQueue(){
    }
    bool try_popk_front(int k,std::list<Type>& c) {
        LockGuard lg(m_mx);
        if (m_deq.empty()) {
            return false;
        }
        for (int i=0;i<k;i++) {
            if(m_deq.empty()) break;
            c.push_back(m_deq.front());
            m_deq.pop_front();
        }
        return true;
    }

    bool try_popk_back(int k,std::list<Type>& c) {
        LockGuard lg(m_mx);
        if (m_deq.empty() ) {
            return false;
        }
        for(int i=0;i<k;i++) {
            if(m_deq.empty()) break;
            c.push_back(m_deq.back());
            m_deq.pop_back();
        }
        return true;
    }

    bool try_pop_back(Type& v) {
        LockGuard lg(m_mx);
        if(m_deq.empty()) {
            return false;
        }
        v = m_deq.back();
        m_deq.pop_back();
        return true;
    }

    bool try_pop_front(Type& v) {
        LockGuard lg(m_mx);
        if(m_deq.empty()) {
            return false;
        }
        v = m_deq.front();
        m_deq.pop_front();
        return true;
    }

    //批量压入
    void pushk_front(std::list<Type>& c) {
        LockGuard lg(m_mx);
        for (auto& v:c) {
            m_deq.push_front(v);
            //m_nput.notify();
        }
        c.clear();
    }

    void pushk_back(std::list<Type>& c) {
      LockGuard lg(m_mx);
      for (auto& v:c) {
        m_deq.push_back(v);
        //m_nput.notify();
        }
        c.clear();
    }

    //单个压入
    void push_back(const Type& v) {
        LockGuard lg(m_mx);
        //if(m_limit != -1 && (int)deq.size() >= m_limit ) return;
        m_deq.push_back(v);
    }

    void push_front(const Type& v) {
        LockGuard lg(m_mx);
        //if(m_limit != -1 && (int)deq.size() >= m_limit ) return;
        m_deq.push_front(v);
    }

    bool empty() const {
        LockGuard lg(m_mx);
        return m_deq.empty();
    }

    void clear() {
        LockGuard lg(m_mx);
        m_deq.clear();
    }

    bool size() {
        LockGuard lg(m_mx);
        return m_deq.size();
    }

private:
    std::deque<Type> m_deq;
    mutable MutexType m_mx;
  };
  
}
