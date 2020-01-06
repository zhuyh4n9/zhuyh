#pragma once

#include <stdint.h>
#include <set>
#include <memory>
#include "../latch/lock.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <vector>

namespace zhuyh
{
  class FdInfo final : public std::enable_shared_from_this<FdInfo>
  {
  public:
    enum Type
      {
	NONE = 0,
	SOCKET = 1,
	PIPE = 2
      };
  public:
    typedef std::shared_ptr<FdInfo> ptr;
    FdInfo(int fd);
    bool isPipe() const
    {
      return _fdType == PIPE;
    }
    
    bool isSocket() const
    {
      return _fdType == SOCKET;
    }
    
    bool isFile() const
    {
      return _fdType == NONE;
    }
    
    void setType(Type type)
    {
      _fdType = type;
    }
    
    bool isBlock() const
    {
      return _blocking;
    }

    bool isNonb() const
    {
      return !_blocking;
    }
    
    int getFd() const
    {
      return _fd;
    }
    
    uint64_t getTimeout(int type) const
    {
      if(type == SO_RCVTIMEO)
	return _rdtimeout;
      else
	return _wrtimeout;
    }
    
    void setTimeout(int type,uint64_t timeout)
    {
      if(type == SO_RCVTIMEO)
	_rdtimeout = timeout;
      else
	_wrtimeout = timeout;
    }
    
    bool isInit() const
    {
      return _inited;
    }
    
    bool isClosed() const
    {
      return _closed;
    }
    
  private:
    int init();
  private:
    //是否阻塞
    bool _blocking = true;
    //读写超时时间
    uint64_t _rdtimeout = (uint64_t)-1;
    uint64_t _wrtimeout = (uint64_t)-1;
    //是否已经关闭
    bool _closed = true;
    //是否初始化
    bool _inited = false;
    //文件描述符
    int _fd = -1;
    //fd类型
    Type _fdType = NONE;
  };

  class FdManager final
  {
  public:
    typedef std::shared_ptr<FdManager> ptr;
    FdInfo::ptr getFdInfo(int fd,bool create = false);
    void delFdInfo(int fd);
  private:
    FdManager();
  public:
    static FdManager::ptr getThis();
  private:
    std::vector<FdInfo::ptr> _fds;
    RWLock _mx;
  };
}