#include "Hook.hpp"
#include <dlfcn.h>
#include "../scheduler/IOManager.hpp"
#include "../scheduler/Scheduler.hpp"
#include "../macro.hpp"
#include "../logUtil.hpp"
#include "../scheduler/TimerManager.hpp"
#include "FdManager.hpp"
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <sys/ioctl.h>

static auto sys_log = GET_LOGGER("system");

namespace zhuyh
{
  ConfigVar<uint64_t>::ptr connect_time_out = Config::lookUp<uint64_t>("connect_time_out",5000,"connect time out");
  extern "C"
  {
    sleep_func sleep_f;
    usleep_func usleep_f;
    nanosleep_func nanosleep_f;
    socket_func socket_f;
    fcntl_func fcntl_f;
    ioctl_func ioctl_f;
    setsockopt_func setsockopt_f;
    accept_func accept_f;
    connect_func connect_f;
    read_func read_f;
    readv_func readv_f;
    send_func send_f;
    sendto_func sendto_f;
    sendmsg_func sendmsg_f;
    write_func write_f;
    writev_func writev_f;
    recv_func recv_f;
    recvfrom_func recvfrom_f;
    recvmsg_func recvmsg_f;
    close_func close_f;
    pipe_func pipe_f;
    pipe2_func pipe2_f;
    dup_func dup_f;
    dup2_func dup2_f;
    dup3_func dup3_f;
  }
  
  static bool& __hook_state__()
  {
    static thread_local bool _state = false;
    return _state;
  }
  void Hook::setHookState(bool state)
  {
    __hook_state__() = state;
  }
  
  bool Hook::isHookEnable()
  {
    return __hook_state__();
  }
}

#define XX(name)					\
  name##_f = (name ## _func) dlsym(RTLD_NEXT,#name);

#define HOOK_FUNC()				\
  XX(sleep);					\
  XX(usleep);					\
  XX(nanosleep);				\
  XX(socket);					\
  XX(fcntl);					\
  XX(ioctl);					\
  XX(setsockopt);				\
  XX(accept);					\
  XX(connect);					\
  XX(read);					\
  XX(readv);					\
  XX(recv);					\
  XX(recvfrom);					\
  XX(recvmsg);					\
  XX(write);					\
  XX(writev);					\
  XX(sendto);					\
  XX(sendmsg);					\
  XX(send);					\
  XX(close);					\
  XX(pipe);					\
  XX(pipe2)					\
  XX(dup)					\
  XX(dup2)					\
  XX(dup3)

struct HookInit
{
  HookInit()
  {
    zhuyh::FdManager::FdMgr::getInstance();
    HOOK_FUNC();
  }
};

#undef XX

//防止一些静态变量在Hook前初始化,构造函数却使用了hook住的函数
void do_init()
{
  //线程安全
  static HookInit __ininter;
}

struct TimerInfo
{
  typedef std::shared_ptr<TimerInfo> ptr;
  int cancled = 0;
};

template<typename OriFunc,typename...Args>
static int do_io(int fd,const char* funcName,OriFunc oriFunc,
		 zhuyh::IOManager::EventType event,int timeout_so,
		 Args&&... args)
{
  if(zhuyh::Hook::isHookEnable() == false)
    return oriFunc(fd,std::forward<Args>(args)...);
  
  auto fdmanager = zhuyh::FdManager::FdMgr::getInstance();
  auto fdInfo= fdmanager->lookUp(fd,false);
  if(fdInfo == nullptr)
    {
      return oriFunc(fd,std::forward<Args>(args)...);
    }
  //exit(1);
  if(fdInfo -> isClosed())
    {
      errno = EBADF;
      return -1;
    }
  if(fdInfo -> isUserNonBlock() == true)
    {
      return oriFunc(fd,std::forward<Args>(args)...);
    }
  if ( fdInfo -> isSocket() == false
       && fdInfo -> isPipe() == false)
    {
      return oriFunc(fd,std::forward<Args>(args)...);
    }
  uint64_t timeout = fdInfo->getTimeout(timeout_so);
  auto scheduler = zhuyh::Scheduler::Schd::getInstance();
  do{
    //LOG_ROOT_WARN() << "call function "<<"do_io<"<<funcName<<">" << " fd = "<<fd;
    int n = 0;
    do{  
      n = oriFunc(fd,std::forward<Args>(args)...);
    }while(n == -1 && errno == EINTR);
    
    if(n == -1 && errno != EAGAIN )
      {
	//LOG_ROOT_ERROR() << "errno = " << errno << " error = "<<strerror(errno);
	return -1;
      }
    if(n >= 0)
      {
	errno = 0;
	return n;
      }
    TimerInfo::ptr tinfo(new TimerInfo());
    std::weak_ptr<TimerInfo> winfo(tinfo);
    zhuyh::Timer::ptr timer = nullptr;
    //LOG_ROOT_INFO()<<funcName<<" timeout : "<<timeout;
    if(timeout != (uint64_t)-1)
      {
	timer.reset(new zhuyh::Timer(0,timeout));
	scheduler->addTimer(timer,[fd,winfo,event,scheduler](){
	    auto t = winfo.lock();
	    if(!t || t->cancled )
	      return;
	    t->cancled = ETIMEDOUT;
	    if(event == zhuyh::IOManager::READ)
	      scheduler->cancleReadEvent(fd);
	    else if(event == zhuyh::IOManager::WRITE)
	      scheduler->cancleWriteEvent(fd);
	  });
      }
    int rt = 0;
    if(event == zhuyh::IOManager::READ)
      rt = scheduler->addReadEvent(fd);
    else if(event == zhuyh::IOManager::WRITE)
      rt = scheduler->addWriteEvent(fd);
    if(rt == 0)
      {
	zhuyh::Fiber::YieldToHold();
	if(timer)
	  timer->cancle();
	if(tinfo->cancled)
	  {
	    //LOG_ROOT_ERROR() << "errno = " <<errno<< " error = "<<strerror(errno);
	    errno = tinfo->cancled;
	    return -1;
	  }
      }
    else
      {
	LOG_ERROR(sys_log) << "Failed to add Event in function : "
			   << funcName <<" rt = "<<rt << " error = "<< strerror(errno);
	if(timer)
	  timer->cancle();
	return -1;
      }
  } while(1);
  
}

extern "C"
{
  unsigned int sleep(unsigned int seconds)
  {
    do_init();
    if(zhuyh::Hook::isHookEnable() == false)
      {
	//LOG_ROOT_ERROR() << "USE ORIGIN";
	return sleep_f(seconds);
      }
    auto scheduler = zhuyh::Scheduler::Schd::getInstance();
    ASSERT(scheduler != nullptr);
    scheduler->addTimer(zhuyh::Timer::ptr(new zhuyh::Timer((time_t)seconds)));
    return 0;
  }
  
  int usleep(useconds_t usec)
  {
    do_init();
    if(zhuyh::Hook::isHookEnable() == false)
      {
	//LOG_ROOT_ERROR() << "USE ORIGIN";
	return usleep_f(usec);
      }
    auto scheduler = zhuyh::Scheduler::Schd::getInstance();
    ASSERT(scheduler != nullptr);
    usec /= 1000;
    scheduler->addTimer(zhuyh::Timer::ptr(new zhuyh::Timer(0,(time_t)usec)));
    return 0;
  }
  
  int nanosleep(const struct timespec *req, struct timespec *rem)
  {
    do_init();
    if(zhuyh::Hook::isHookEnable() == false)
      return nanosleep_f(req,rem);
    auto scheduler = zhuyh::Scheduler::Schd::getInstance();
    ASSERT(scheduler != nullptr);
    time_t sec = req->tv_sec;
    time_t nsec = req->tv_nsec;
    if(nsec <0 || nsec > 1000000000 || sec < 0 || sec > 1000000000)
      {
	errno = EINVAL;
	return -1;
      }
    nsec /=1000000;
    scheduler->addTimer(zhuyh::Timer::ptr(new zhuyh::Timer(sec,(time_t)nsec)));
    return 0;
    }
  
  int socket(int domain, int type, int protocol)
  {
    do_init();
    if(zhuyh::Hook::isHookEnable() == false)
      return socket_f(domain,type,protocol);
    int fd = socket_f(domain,type,protocol);
    if(fd == -1)
      return fd;
    zhuyh::FdManager::ptr fdmanager = zhuyh::FdManager::FdMgr::getInstance();
    fdmanager->lookUp(fd,true);
    return fd;
  }
  
  int connect(int sockfd, const struct sockaddr *addr,
	      socklen_t addrlen)
  {
    do_init();
    if(zhuyh::Hook::isHookEnable() == false)
      return connect_f(sockfd,addr,addrlen);
    zhuyh::FdManager::ptr fdmanager = zhuyh::FdManager::FdMgr::getInstance();
    zhuyh::FdInfo::ptr fdInfo = fdmanager->lookUp(sockfd,false);
    if(fdInfo == nullptr)
      {
	return connect_f(sockfd,addr,addrlen);
      }
    if(fdInfo->isClosed() == true)
      {
	errno = EBADF;
	return -1;
      }
    //如果是用户设置的非阻塞则交给用户处理
    if(fdInfo->isSocket() == false ||
       fdInfo->isUserNonBlock() == true)
      {
	return connect_f(sockfd,addr,addrlen);
      }
       
    int rt = connect_f(sockfd,addr,addrlen);
    if(rt == 0)
      {
	return 0;
      }
    else if(rt != -1 || errno != EINPROGRESS)
      {
	return rt;
      }
    uint64_t timeout = zhuyh::connect_time_out->getVar();
    zhuyh::Scheduler* scheduler = zhuyh::Scheduler::Schd::getInstance();
    zhuyh::Timer::ptr timer = nullptr;
    std::shared_ptr<TimerInfo> tinfo(new TimerInfo());
    std::weak_ptr<TimerInfo> winfo(tinfo);
    //LOG_ROOT_INFO() << timeout;
    if(timeout != (uint64_t)-1)
      {
	timer.reset(new zhuyh::Timer(0,timeout));
	scheduler->addTimer(timer,[winfo,sockfd,scheduler](){
	    //LOG_ROOT_INFO() << "cancling";
	    auto t = winfo.lock();
	    if(!t || t->cancled)
	      {
		return ;
	      }
	    t->cancled = ETIMEDOUT;
	    //LOG_ROOT_INFO() << "cancled";
	    scheduler->cancleWriteEvent(sockfd);
	  });
      }
    rt = scheduler->addWriteEvent(sockfd);
    if(rt == 0)
      {
	//LOG_ROOT_INFO() << "Switching...";
	zhuyh::Fiber::YieldToHold();
	//LOG_ROOT_INFO() << "Switched";
	if(timer)
	  {
	    //  LOG_ROOT_INFO() << sockfd;
	    timer->cancle();
	  }
	if(tinfo->cancled)
	  {
	    errno = tinfo->cancled;
	    return -1;
	  }
      }
    else
      {
	if(timer)
	  {
	    timer->cancle();
	  }
	LOG_ERROR(sys_log) << "Failed to add Event";
	return -1;
      }
    int error = 0;
    socklen_t len = sizeof(int);
    if(getsockopt(sockfd,SOL_SOCKET,SO_ERROR,&error,&len) == -1)
      {
	return -1;
      }
    if(error == 0)
      {
	return 0;
      }
    else
      {
	errno = error;
	return -1;
      }
  }
  int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
  {
    do_init();
    int fd = do_io(sockfd,"accept",accept_f,zhuyh::IOManager::READ,
		 SO_RCVTIMEO,addr,addrlen);
    if(fd >= 0)
      {
	auto fdmanager = zhuyh::FdManager::FdMgr::getInstance();
	fdmanager->lookUp(fd,true);
      }
    return fd;
  }

  ssize_t read(int fd, void *buf, size_t count)
  {
    do_init();
    return do_io(fd,"read",read_f,zhuyh::IOManager::READ,
		 SO_RCVTIMEO,buf,count);
  }
  
  ssize_t readv(int fd, const struct iovec *iov, int iovcnt)
  {
    do_init();
    return do_io(fd,"readv",readv_f,zhuyh::IOManager::READ,
		 SO_RCVTIMEO,iov,iovcnt);
  }
  
  ssize_t recv(int sockfd, void *buf, size_t len, int flags)
  {
    do_init();
    return do_io(sockfd,"recv",recv_f,zhuyh::IOManager::READ,
		 SO_RCVTIMEO,buf,len,flags);
  }

  ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
		   struct sockaddr *src_addr, socklen_t* addrlen)
  {
    do_init();
    return do_io(sockfd,"recvfrom",recvfrom_f,zhuyh::IOManager::READ,
		 SO_RCVTIMEO,buf,len,flags,src_addr,addrlen);
  }

  ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags)
  {
    do_init();
    return do_io(sockfd,"recvfrom",recvmsg_f,zhuyh::IOManager::READ,
		 SO_RCVTIMEO,msg,flags);
  }
  
  ssize_t write(int fd, const void *buf, size_t count)
  {
    do_init();
    return do_io(fd,"write",write_f,zhuyh::IOManager::WRITE,
		 SO_SNDTIMEO,buf,count);
  }

  ssize_t writev(int fd, const struct iovec *iov, int iovcnt)
  {
    do_init();
    return do_io(fd,"writev",writev_f,zhuyh::IOManager::WRITE,
		 SO_SNDTIMEO,iov,iovcnt);
  }
  
  ssize_t send(int sockfd, const void *buf, size_t len, int flags)
  {
    do_init();
    return do_io(sockfd,"send",send_f,zhuyh::IOManager::WRITE,
		 SO_SNDTIMEO,buf,len,flags);
  }

  ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
		 const struct sockaddr *dest_addr, socklen_t addrlen)
  {
    do_init();
    return do_io(sockfd,"sendto",sendto_f,zhuyh::IOManager::WRITE,
		 SO_SNDTIMEO,buf,len,flags,dest_addr,addrlen);
  }

  ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags)
  {
    do_init();
    return do_io(sockfd,"sendmsg",sendmsg_f,zhuyh::IOManager::WRITE,
		 SO_SNDTIMEO,msg,flags);
  }

  int close(int fd)
  {
    do_init();
    if(zhuyh::Hook::isHookEnable() == false)
      {
	return close_f(fd);
      }
    auto fdmanager = zhuyh::FdManager::FdMgr::getInstance();
    auto fdInfo = fdmanager->lookUp(fd,false);
    if(fdInfo)
      {
	auto scheduler = zhuyh::Scheduler::Schd::getInstance();
	scheduler->cancleAllEvent(fd);
	fdmanager->del(fd);
      }
    return close_f(fd);
  }

  int fcntl(int fd, int cmd, ... /* arg */ )
  {
    do_init();
    va_list va;
    va_start(va,cmd);
    switch(cmd)
      {
      case F_SETFL :
	{
	  int arg = va_arg(va,int);
	  va_end(va);
	  auto fdInfo = zhuyh::FdManager::FdMgr::getInstance()->lookUp(fd,false);
	  if(fdInfo == nullptr || fdInfo->isClosed() || !fdInfo->isSocket())
	    {
	      return fcntl_f(fd,cmd,arg);
	    }
	  fdInfo->setUserNonBlock(arg & O_NONBLOCK);
	  if(fdInfo->isSysNonBlock())
	    {
	      arg |= O_NONBLOCK;
	    }
	  else
	    {
	      arg &= ~O_NONBLOCK;
	    }
	  return fcntl_f(fd,cmd,arg);
	}
	break;
      case F_GETFL :
	{
	  va_end(va);
	  int arg = fcntl_f(fd,cmd);
	  auto fdInfo = zhuyh::FdManager::FdMgr::getInstance()->lookUp(fd,false);
	  if(fdInfo == nullptr || fdInfo->isClosed() || !fdInfo->isSocket())
	    {
	      return arg;
	    }
	  if(fdInfo->isUserNonBlock())
	    {
	      return arg | O_NONBLOCK;
	    }
	  else
	    {
	      return arg & ~ O_NONBLOCK;
	    }
	}
	break;
      case F_DUPFD :
      case F_DUPFD_CLOEXEC :
      case F_SETFD :
      case F_SETOWN :
      case F_SETSIG :
      case F_SETLEASE :
      case F_NOTIFY :
#ifdef F_SETPIPE_SZ
      case F_SETPIPE_SZ :
#endif
	{
	  int arg = va_arg(va,int);
	  va_end(va);
	  return fcntl_f(fd,cmd,arg);
	}
	break;
      case F_GETFD :
      case F_GETOWN :
      case F_GETSIG :
      case F_GETLEASE :
#ifdef F_GETPIPE_SZ
      case F_GETPIPE_SZ :
#endif
	{
	  va_end(va);
	  return fcntl_f(fd,cmd);
	}
	break;
      case F_SETLK :
      case F_SETLKW :
      case F_GETLK :
	{
	  struct flock* arg = va_arg(va,struct flock*);
	  va_end(va);
	  return fcntl(fd,cmd,arg);
	}
	break;
      case F_GETOWN_EX :
      case F_SETOWN_EX :
	{
	  struct f_owner_exlock* arg = va_arg(va,struct f_owner_exlock*);
	  va_end(va);
	  return fcntl(fd,cmd,arg);
	}
	break;
      default:
	va_end(va);
	return fcntl(fd,cmd);
      }
  }
  
  int ioctl(int fd, unsigned long request, ...)
  {
    do_init();
    va_list va;
    va_start(va,request);
    void* arg = va_arg(va,void*);
    va_end(va);
    if(request == FIONBIO)
      {
	bool setNb = !!*(int*)arg;
	auto fdInfo = zhuyh::FdManager::FdMgr::getInstance()->lookUp(fd,false);
	if(fdInfo == nullptr || fdInfo->isClosed() || !fdInfo->isSocket())
	  return ioctl_f(fd,request,setNb);
	fdInfo->setUserNonBlock(setNb);
      }
    return ioctl_f(fd,request,arg); 
  }
  
  int setsockopt(int sockfd, int level, int optname,
		 const void *optval, socklen_t optlen)
  {
    do_init();
    //LOG_ROOT_INFO() << "Set Sockoption1";
    int rt = setsockopt_f(sockfd, level, optname, optval, optlen);
    if(zhuyh::Hook::isHookEnable() == false)
      return rt;
    if(level == SOL_SOCKET)
      {
	if(optname == SO_RCVTIMEO
	   || optname == SO_SNDTIMEO)
	  {
	    //LOG_ROOT_INFO() << "Set Sockoption2";
	    auto fdInfo = zhuyh::FdManager::FdMgr::getInstance()->lookUp(sockfd,false);
	    //肯会有线程安全问题
	    if(fdInfo)
	      {
		//LOG_ROOT_INFO() << "Set Sockoption3r";
		const timeval& tv = *(const timeval*)optval;
		uint64_t millsec = tv.tv_sec*1000 + tv.tv_usec / 1000;
		fdInfo -> setTimeout(optname,millsec);
	      }
	  }
      }
    return rt;
  }

  //
  int pipe(int pipefd[2])
  {
    do_init();
    int rt = pipe_f(pipefd);
    if(zhuyh::Hook::isHookEnable() == false)
      return  rt;
    //LOG_ROOT_INFO() << "called pipe";
    if(rt < 0)
      return rt;
    auto mgr = zhuyh::FdManager::FdMgr::getInstance();
    mgr->lookUp(pipefd[0],true);
    mgr->lookUp(pipefd[1],true);
    return rt;
  }

  int pipe2(int pipefd[2],int flags)
  {
    do_init();
    int rt = pipe2_f(pipefd,flags);
    if(zhuyh::Hook::isHookEnable() == false)
      return  rt;
    //LOG_ROOT_INFO() << "called pipe2";
    if(rt < 0)
      return rt;
    auto mgr = zhuyh::FdManager::FdMgr::getInstance();
    //用户设置了非阻塞则交个用户处理
    auto fdInfo = mgr->lookUp(pipefd[0],true);
    fdInfo->setUserNonBlock(flags & O_NONBLOCK);
    auto fdInfo2 =mgr->lookUp(pipefd[1],true);
    fdInfo2->setUserNonBlock(flags & O_NONBLOCK);
    
    return rt;
  }

  int dup(int oldfd)
  {
    do_init();
    int newfd = dup_f(oldfd);
    if(zhuyh::Hook::isHookEnable() == false)
      return newfd;
    //LOG_ROOT_INFO() << "called dup";
    if(newfd < 0)
      return newfd;
    auto mgr = zhuyh::FdManager::FdMgr::getInstance();
    mgr->lookUp(newfd,true);
    return newfd;
  }

  int dup2(int oldfd,int newfd)
  {
    do_init();
    int rt = dup2(oldfd,newfd);
    if(zhuyh::Hook::isHookEnable() == false)
      return newfd;
    //LOG_ROOT_INFO() << "called dup2";
    if(rt < 0)
      return rt;
    auto mgr = zhuyh::FdManager::FdMgr::getInstance();
    mgr->lookUp(newfd,true);
    return rt;
  }

  //不一定所有系统都支持
  int dup3(int oldfd,int newfd,int flags)
  {
    do_init();
    if(dup3_f == nullptr)
      {
	errno = EPERM;
	return -1;
      }
    if(zhuyh::Hook::isHookEnable() == false)
      return newfd;
    //    LOG_ROOT_INFO() << "called dup3";
    int rt = dup3_f(oldfd,newfd,flags);
    if(rt < 0)
      return rt;
    auto mgr = zhuyh::FdManager::FdMgr::getInstance();
    mgr->lookUp(newfd,true);
    return rt;
  }
}
