#include "Hook.hpp"
#include <dlfcn.h>
#include "../scheduler/IOManager.hpp"
#include "../scheduler/Scheduler.hpp"
#include "../macro.hpp"
#include "../logUtil.hpp"
#include "../scheduler/TimerManager.hpp"
#include "FdManager.hpp"
#include <stdlib.h>

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
  
  bool& __hook_state__()
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

template<class func>
void do_init(func fn)
{
  if(fn == nullptr)
    __hook_initer__();
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
    do_init(sleep_f);
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
    do_init(usleep_f);
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
    do_init(nanosleep_f);
    LOG_ROOT_INFO() << "START";
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
    do_init(socket_f);
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
    do_init(connect_f);
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
	scheduler->addTimer(timer,[winfo,sockfd](){
	    auto t = winfo.lock();
	    if(!t || t->cancled)
	      {
		return ;
	      }
	    zhuyh::Scheduler* scheduler = zhuyh::Scheduler::getThis();
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
    do_init(accept_f);
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
    do_init(read_f);
    return do_io(fd,"read",read_f,zhuyh::IOManager::READ,
		 SO_RCVTIMEO,buf,count);
  }
  
  ssize_t readv(int fd, const struct iovec *iov, int iovcnt)
  {
    do_init(readv_f);
    return do_io(fd,"readv",readv_f,zhuyh::IOManager::READ,
		 SO_RCVTIMEO,iov,iovcnt);
  }
  
  ssize_t recv(int sockfd, void *buf, size_t len, int flags)
  {
    do_init(recv_f);
    return do_io(sockfd,"recv",recv_f,zhuyh::IOManager::READ,
		 SO_RCVTIMEO,buf,len,flags);
  }

  ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
		   struct sockaddr *src_addr, socklen_t* addrlen)
  {
    do_init(recvfrom_f);
    return do_io(sockfd,"recvfrom",recvfrom_f,zhuyh::IOManager::READ,
		 SO_RCVTIMEO,buf,len,flags,src_addr,addrlen);
  }

  ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags)
  {
    do_init(recvmsg_f);
    return do_io(sockfd,"recvfrom",recvmsg_f,zhuyh::IOManager::READ,
		 SO_RCVTIMEO,msg,flags);
  }
  
  ssize_t write(int fd, const void *buf, size_t count)
  {
    do_init(write_f);
    return do_io(fd,"write",write_f,zhuyh::IOManager::WRITE,
		 SO_SNDTIMEO,buf,count);
  }

  ssize_t writev(int fd, const struct iovec *iov, int iovcnt)
  {
    do_init(writev_f);
    return do_io(fd,"writev",writev_f,zhuyh::IOManager::WRITE,
		 SO_SNDTIMEO,iov,iovcnt);
  }
  
  ssize_t send(int sockfd, const void *buf, size_t len, int flags)
  {
    do_init(send_f);
    return do_io(sockfd,"send",send_f,zhuyh::IOManager::WRITE,
		 SO_SNDTIMEO,buf,len,flags);
  }

  ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
		 const struct sockaddr *dest_addr, socklen_t addrlen)
  {
    do_init(sendto_f);
    return do_io(sockfd,"sendto",sendto_f,zhuyh::IOManager::WRITE,
		 SO_SNDTIMEO,buf,len,flags,dest_addr,addrlen);
  }

  ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags)
  {
    do_init(sendmsg_f);
    return do_io(sockfd,"sendmsg",sendmsg_f,zhuyh::IOManager::WRITE,
		 SO_SNDTIMEO,msg,flags);
  }

  int close(int fd)
  {
    do_init(close_f);
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
    do_init(fcntl_f);
    return 0;
  }
  
  int ioctl(int fd, unsigned long request, ...)
  {
    do_init(ioctl_f);
    return 0;
  }
  
  int setsockopt(int sockfd, int level, int optname,
		 const void *optval, socklen_t optlen)
  {
    do_init(setsockopt_f);
    return 0;
  }
  
}
