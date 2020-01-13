#include "FdManager.hpp"
#include "Hook.hpp"
#include <sys/stat.h>
#include <sys/types.h>
#include "../latch/lock.hpp"
#include <fcntl.h>
#include "../macro.hpp"
#include "../logUtil.hpp"

namespace zhuyh
{

  FdInfo::FdInfo(int fd)
    :_sysBlocking(true),
     _userBlocking(true),
     _rdtimeout((uint64_t)-1),
     _wrtimeout((uint64_t)-1),
     _closed(true),
     _inited(false),
     _fd(fd),
     _fdType(NONE)
  {
    init();
  }
  
  int FdInfo::init()
  {
    if(_inited == true) return 0;
    _rdtimeout = (uint64_t)-1;
    _wrtimeout = (uint64_t)-1;
    struct stat _stat;
    //fd类型
    if(fstat(_fd,&_stat) != -1)
      {
	if(S_ISSOCK(_stat.st_mode) )
	  {
	    _fdType = SOCKET;
	  }
      }
    //设置非阻塞,目前只处理Socket描述符
    if(_fdType != NONE)
      {
	int flags = 0;
	if((flags = fcntl_f(_fd,F_GETFL))!=-1)
	  {
	    if( (flags & O_NONBLOCK) == 0)
	      {
		int rt = fcntl(_fd,F_SETFL,flags | O_NONBLOCK);
		if(rt < 0)
		  return -1;
	      }
	  }
	_sysBlocking = false;
      }
    else
      {
	_sysBlocking = true;
      }
    _closed = false;
    _inited = true;
    return 0;
  }
  
  FdManager::FdManager()
  {
    _fds.resize(1024);
  }
  
  FdInfo::ptr FdManager::lookUp(int fd,bool create)
  {
    ASSERT(fd >=0);
    {
      RDLockGuard lg(_mx);
      if((unsigned)fd >= _fds.size())
	{
	  if(!create) return nullptr;	
	}
      else
	{
	  if(!create || _fds[fd] == nullptr)
	    return _fds[fd];
	}
    }
    //_fds[fd] == nullptr 且 create == true
    WRLockGuard lg(_mx);
    if((unsigned)fd >= _fds.size())
      {
	_fds.resize(fd*2);
      }
    _fds[fd].reset(new FdInfo(fd));
    return _fds[fd];
  }

  void FdManager::del(int fd)
  {
    ASSERT(fd >= 0);
    WRLockGuard lg(_mx);
    if((unsigned)fd >= _fds.size())
      throw std::out_of_range("fd doesn't exist");
    _fds[fd].reset();
  }

  FdManager::ptr FdManager::getThis()
  {
    static FdManager::ptr fdManager(new FdManager());
    return fdManager;
  }
  
}
