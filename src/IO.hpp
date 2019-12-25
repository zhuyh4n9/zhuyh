#pragma once

#include <unistd.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>

namespace zhuyh
{
  class IOHandler
  {
  public:
    IOHandler(const std::string& path,size_t flags,size_t sflags);
    ~IOHandler();
    //是否可读
    bool readable();
    //是否可写
    bool writable();

    virtual bool write();
    virtual bool read();
    virtual void close();
    virtual void open();
    
    bool setHookOn();
    bool unsetHookOn();
  private:
    bool setBlock();
    bool setNonb();
  protected:
    //是否开启IO hook,开启之后无法设置阻塞
    bool _hookOn = false;
    int _fd = -1;
  };
};
