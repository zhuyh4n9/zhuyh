#include "FdManager.hpp"
#include "Hook.hpp"
#include <sys/stat.h>
#include <sys/types.h>
#include "latch/lock.hpp"
#include <fcntl.h>
#include "macro.hpp"
#include "logUtil.hpp"

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
	if(S_ISFIFO(_stat.st_mode))
	  {
	    //LOG_ROOT_INFO() << "IS PIPE";
	    _fdType = PIPE;
	  }
      } 
    //设置非阻塞,目前只处理Socket描述符
    if(_fdType != NONE)
      {
	int flags = 0;
	if((flags = fcntl_f(_fd,F_GETFL))!=-1)
	  {
	    //LOG_ROOT_ERROR() << "Set Nonb Success, rt : " <<flags << "fd : "<<_fd<<std::endl;
	    //LOG_ROOT_ERROR() << "Set Nonb Success, rt : " <<flags << "fd : "<<_fd<<std::endl;
	    int rt = fcntl_f(_fd,F_SETFL,flags | O_NONBLOCK);
	    //LOG_ROOT_ERROR() << "Set Nonb Success, rt : " <<flags << "fd : "<<_fd<<std::endl;
	    if(rt < 0)
	      return -1;
	  }
	else
	  {
	    LOG_ROOT_ERROR() << "Failed to GET flag , rt : " <<flags << "fd : "<<_fd;
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
    _fds.resize(4096);
  }
  
  FdInfo::ptr FdManager::lookUp(int fd,bool create)
  {
    if(fd < 0 ) return nullptr;
    LockGuard lg(m_mx);
    if((unsigned)fd >= _fds.size())
      {
	if(!create) return nullptr;	
      }
    else
      {
	if(create == false)
	  return _fds[fd];
      }
    //lg.unlock();
    //_fds[fd] == nullptr 且 create == true
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
    LockGuard lg(m_mx);
    if((unsigned)fd >= _fds.size())
      throw std::out_of_range("fd doesn't exist");
    _fds[fd].reset();
  }

  // FdManager::ptr FdManager::getThis()
  // {
  //   static FdManager::ptr fdManager(new FdManager());
  //   return fdManager;
  // }
  
}
