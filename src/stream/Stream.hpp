#pragma once

#include <memory>
#include "../bytearray/ByteArray.hpp"

namespace zhuyh
{
  class Stream
  {
  public:
    typedef std::shared_ptr<Stream> ptr;
    virtual ~Stream() {};
    
    virtual int read(void* buff,size_t length) = 0;
    virtual int read(ByteArray::ptr ba,size_t length) = 0;
    //读固定长度
    virtual int readFixSize(void* buff,size_t length);
    virtual int readFixSize(ByteArray::ptr ba,size_t length);
    
    virtual int write(const void* buff,size_t length) = 0;
    virtual int write(ByteArray::ptr ba,size_t length) = 0;
    //写固定长度
    virtual int writeFixSize(const void* buff,size_t length);
    virtual int writeFixSize(ByteArray::ptr ba,size_t length);

    virtual void close() = 0;
  };
  
}
