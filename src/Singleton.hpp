#pragma once

#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <memory>

namespace zhuyh
{
  //C++11静态局部变量初始化线程安全
  template<class T>
  class Singleton
  {
  public:
    static T* getInstance()
    {
      static T val;
      return &val;
    }
  };
  
  template<class T>
  class SingletonPtr
  {
  public:
    static std::shared_ptr<T> getInstance()
    {
      static std::shared_ptr<T> p(new T());
      return p;
    }
  };
  
}
