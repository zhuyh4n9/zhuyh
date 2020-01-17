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
  XX(close);					

struct HookInit
{
  HookInit()
  {
    static std::atomic_flag initTag{ATOMIC_FLAG_INIT};
    //防止被初始化多次
    if(!initTag.test_and_set())
      {
	zhuyh::FdManager::getThis();
	HOOK_FUNC();
      }  
  }
};

#undef XX

static HookInit& __hook_initer__()
{
  static HookInit __ininter;
  return __ininter;
}

//防止一些静态变量在Hook前初始化,构造函数却使用了hook住的函数
void do_init()
{
  static bool inited = false;
  if(!inited)
    {
      inited = true;
      __hook_initer__();
    }
}

struct TimerInfo
{
  typedef std::shared_ptr<TimerInfo> ptr;
  bool cancled = false;
};

template<typename OriFunc,typename...Args>
static int do_io(int fd,const char* funcName,OriFunc oriFunc,
		 zhuyh::IOManager::EventType event,int timeout_so,
		 Args&&... args)
{
  if(zhuyh::Hook::isHookEnable() == false)
    return oriFunc(fd,std::forward<Args>(args)...);
  auto fdmanager = zhuyh::FdManager::getThis();
  auto fdInfo= fdmanager->lookUp(fd,false);
  if(fdInfo == nullptr)
    {
      return oriFunc(fd,std::forward<Args>(args)...);
    }
  if(fdInfo -> isClosed())
    {
      errno = EBADF;
      return -1;
    }
  if(fdInfo -> isUserNonBlock() == true
     || fdInfo -> isSocket() == false)
    {
      return oriFunc(fd,std::forward<Args>(args)...);
    }
  TimerInfo::ptr tinfo(new TimerInfo);
  uint64_t timeout = fdInfo->getTimeout(timeout_so);
  auto scheduler = zhuyh::Scheduler::getThis();
  do{
    int n = 0;
    do{  
      n = oriFunc(fd,std::forward<Args>(args)...);
    }while(n == -1 && errno == EINTR);
    
    if(n == -1 && (errno != EAGAIN && errno != EWOULDBLOCK))
      {
	return -1;
      }
    if(n >= 0)
      return n;
    std::weak_ptr<TimerInfo> winfo(tinfo);
    zhuyh::Timer::ptr timer = nullptr;
    if(timeout != (uint64_t)-1)
      {
	timer.reset(new zhuyh::Timer(0,timeout));
	scheduler->addTimer(timer,[fd,winfo,event,scheduler](){
	    auto t = winfo.lock();
	    if(!t || t->cancled )
	      return;
	    errno = ETIMEDOUT;
	    if(event == zhuyh::IOManager::READ)
	      scheduler->cancleReadEvent(fd);
	    else if(event == zhuyh::IOManager::WRITE)
	      scheduler->cancleWriteEvent(fd);
	  });
      }
    int rt = 0;
    if(event == zhuyh::IOManager::READ)
      rt = scheduler->cancleReadEvent(fd);
    else if(event == zhuyh::IOManager::WRITE)
      rt = scheduler->cancleWriteEvent(fd);
    if(rt == 0)
      {
	zhuyh::Fiber::YieldToSwitch();
	if(timer)
	  timer->cancle();
	if(tinfo->cancled)
	  {
	    errno = tinfo->cancled;
	    return -1;
	  }
      }
    else
      {
	LOG_ERROR(sys_log) << "Failed to add Event in function : "<< funcName;
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
    auto scheduler = zhuyh::Scheduler::getThis();
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
    auto scheduler = zhuyh::Scheduler::getThis();
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
    auto scheduler = zhuyh::Scheduler::getThis();
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
    zhuyh::FdManager::ptr fdmanager = zhuyh::FdManager::getThis();
    fdmanager->lookUp(fd,true);
    return fd;
  }
  
  int connect(int sockfd, const struct sockaddr *addr,
	      socklen_t addrlen)
  {
    do_init();
    if(zhuyh::Hook::isHookEnable() == false)
      return connect_f(sockfd,addr,addrlen);
    zhuyh::FdManager::ptr fdmanager = zhuyh::FdManager::getThis();
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
    else if(rt != -1 || errno == EINPROGRESS)
      {
	return rt;
      }
    uint64_t timeout = zhuyh::connect_time_out->getVar();
    zhuyh::Scheduler* scheduler = zhuyh::Scheduler::getThis();
    zhuyh::Timer::ptr timer = nullptr;
    std::shared_ptr<TimerInfo> tinfo(new TimerInfo());
    std::weak_ptr<TimerInfo> winfo(tinfo);
    
    if(timeout != (uint64_t)-1)
      {
	scheduler->addTimer(timer,[winfo,sockfd,scheduler](){
	    auto t = winfo.lock();
	    if(!t || t->cancled)
	      {
		return ;
	      }
	    t->cancled = ETIMEDOUT;
	    scheduler->cancleWriteEvent(sockfd);
	  });
      }
    rt = scheduler->addWriteEvent(sockfd);
    if(rt == 0)
      {
	zhuyh::Fiber::YieldToSwitch();
	if(timer)
	  {
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
	auto fdmanager = zhuyh::FdManager::getThis();
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
    auto scheduler = zhuyh::Scheduler::getThis();
    auto fdmanager = zhuyh::FdManager::getThis();
    auto fdInfo = fdmanager->lookUp(fd,false);
    if(fdInfo)
      {
	fdInfo->close();
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
	  auto fdInfo = zhuyh::FdManager::getThis()->lookUp(fd,false);
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
	  auto fdInfo = zhuyh::FdManager::getThis()->lookUp(fd,false);
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
	auto fdInfo = zhuyh::FdManager::getThis()->lookUp(fd,false);
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
    int rt = setsockopt_f(sockfd, level, optname, optval, optlen);
    if(zhuyh::Hook::isHookEnable() == false)
      return rt;
    if(level == SOL_SOCKET)
      {
	if(optname == SO_RCVTIMEO
	   || optname == SO_SNDTIMEO)
	  {
	    auto fdInfo = zhuyh::FdManager::getThis()->lookUp(sockfd,false);
	    if(fdInfo)
	      {
		const timeval& tv = *(const timeval*)optval;
		uint64_t millsec = tv.tv_sec*1000 + tv.tv_usec / 1000;
		fdInfo -> setTimeout(optname,millsec);
	      }
	  }
      }
    return rt;
  }
  
}
