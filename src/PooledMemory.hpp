/*
 * 内存池,版本1
 * 未完成
 */
#pragma once

#include <sys/mman.h>
#include <stdlib.h>
#include "latch/lock.hpp"

namespace zhuyh
{
  //协程栈专用内存池,在mmap映射的段上分配空间,起始地址一定是4k字节对齐
  class FiberStackPool
  {
  public:
    typedef char Byte;
    void* alloc(unsigned int size)
    {
      
    }
    void dealloc(void* addr);
    
  private:
    union MemItem
    {
      void* next;
      void* buf;
    };
    
    static std::unordered_map<uint64_t>& mp()
    {
      std::unordered_map<uint64_t> _mp;
      return _mp;
    }
    static unsigned int& Total()
    {
      thread_local unsigned _total = 0;
      return _total;
    }
    static unsigned int& Used()
    {
      thread_local unsigned _used = 0;
      return _used;
    }
    static unsigned int& Bucket()
    {
      thread_local unsigned  _bucket = 0;
      return 0;
    }
    static unsigned int& maxBucket()
    {
      thread_local unsigned int _maxbucket;
      return _maxbucket;
    }
  };

  //通用内存池,在堆上分配内存
  template<class T>
  class Pool
  {
    
  };
};
