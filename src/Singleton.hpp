#pragma once

#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <memory>

namespace zhuyh
{
  template<class T>
  class Singleton
  {
  private:
    static T*& getVal()
    {
      static T* _val;
      return _val;
    }
  public:
    static void init()
    {
      getVal() = new T();
    }
    static T* getInstance()
    {
      static pthread_once_t once_flag;
      if(pthread_once(&once_flag,Singleton<T>::init) < 0)
	{
	  std::cout  << "pthread_once error : "<< strerror(errno);
	  exit(1);
	}
      return getVal();
    }
  };
  //智能指针版本
  template<class T>
  class SingletonPtr
  {
  private:
    static std::shared_ptr<T>& getVal()
    {
      static std::shared_ptr<T> m_val;
      return m_val;
    }
  public:
    static void init()
    {
      getVal().reset(new T());
    }
    static std::shared_ptr<T> getInstance()
    {
      static pthread_once_t once_flag;
      if(pthread_once(&once_flag,SingletonPtr<T>::init) < 0)
	{
	  std::cout << "pthread_once error : "<< strerror(errno);
	  exit(1);
	}
      //  std::shared_ptr<T> val = getVal();
      return getVal();
    }
  };
  
}
