#include "Socket.hpp"
#include "../netio/FdManager.hpp"
#include <netinet/tcp.h>
namespace zhuyh
{
  Socket::Socket(int family,int type,int protocol)
    :m_sockfd(-1),
     m_family(family),
     m_protocol(protocol),
     m_type(type),
     m_connect(false),
     m_localAddr(nullptr),
     m_remoteAddr(nullptr)
  {
  }
  Socket::~Socket()
  {
    close();
  }
  uint64_t Socket::getSendTimeout() const
  {
    if(m_sockfd < 0 ) return (uint64_t)-1;
    auto fdinfo = FdManager::FdMgr::getInstance()->lookUp(m_sockfd);
    uint64_t to = (uint64_t)-1;
    if(fdinfo)
      to = fdinfo->getTimeout(SO_SNDTIMEO);
    return to;
  }

  bool  Socket::setSendTimeout(uint64_t to)
  {
    if(m_sockfd < 0 ) return false;
    struct timeval tv;
    tv.tv_sec = to / 1000;
    tv.tv_usec = (to % 1000)*1000;
    int rt = setSockOption(SOL_SOCKET,SO_SNDTIMEO,tv);
    return !rt;
  }
  uint64_t Socket::getRecvTimeout() const
  {
    if(m_sockfd < 0 ) return (uint64_t)-1;
    auto fdinfo = FdManager::FdMgr::getInstance()->lookUp(m_sockfd);
    uint64_t to = (uint64_t)-1;
    if(fdinfo)
      to = fdinfo->getTimeout(SO_RCVTIMEO);
    return to;
  }

  bool  Socket::setRecvTimeout(uint64_t to)
  {
    if(m_sockfd < 0) return false;
    struct timeval tv;
    tv.tv_sec = to / 1000;
    tv.tv_usec = (to % 1000)*1000;
    int rt = setSockOption(SOL_SOCKET,SO_RCVTIMEO,tv);
    //LOG_ROOT_INFO() << " setRecvTimeout rt : "<<rt;
    return rt;
  }

  bool Socket::setSockOption(int level, int optname,
			     const void *optval, socklen_t optlen)
  {
    //LOG_ROOT_INFO() << " setSockOption";
    int rt = setsockopt(m_sockfd,level,optname,optval,optlen);
    if(rt)
      {
	LOG_ERROR(sys_log) << "setsockopt failed , error : "
			   <<strerror(errno) << " errno : "<<errno;
	return false;
      }
    return true;
  }

  bool Socket::getSockOption(int level, int optname,
			     void *res, socklen_t* optlen)
  {
    int rt = getsockopt(m_sockfd,level,optname,res,optlen);
    if(rt)
      {
	LOG_ERROR(sys_log) << "setsockopt failed , error : "
			   <<strerror(errno) << " errno : "<<errno;
	return false;
      }
    return true;
  }
  //初始化一个sockfd
  bool Socket::init(int sockfd)
  {
    if(m_sockfd != -1) return false;
    if(sockfd < 0) return false;
    auto fdInfo = FdManager::FdMgr::getInstance()->lookUp(sockfd);
    if(fdInfo && fdInfo->isSocket() && !fdInfo->isClosed())
      {
	m_sockfd = sockfd;
	m_connect = true;
	int rt = initSock();
	if(!rt) return false;
	if(getRemoteAddr() == nullptr) return false;
	if(getLocalAddr() == nullptr) return false;
	return true;
      }
    return false;
  }

  //设置套接字选项 : 关闭Nagle算法，设置地址复用
  bool Socket::initSock()
  {
    int val = 1;
    setSockOption(SOL_SOCKET,SO_REUSEADDR,val);
    if(m_type == SOCK_STREAM)
      {
	//取消Nagle
	setSockOption(IPPROTO_TCP,TCP_NODELAY,val);
      }
    return true;
  }
  //创建一个socket,并且初始化
  bool Socket::init()
  {
    m_sockfd = socket(m_family,m_type,m_protocol);
    if(m_sockfd == -1)
      {
	LOG_ERROR(sys_log) << "socket error : " << strerror(errno) << " errno : "<<errno;
	return false;
      }
    initSock();
    return true;
  }
  bool Socket::bind(IAddress::ptr addr)
  {
    if(m_sockfd < 0)
      {
	init();
	if(m_sockfd < 0) return false;
      }
    if(addr->getFamily() != m_family)
      {
	LOG_ERROR(sys_log) << "family type Substitute error , "
	  "required : "<<getFamilyType(m_family) << " actual : "
			   <<getFamilyType(addr->getFamily());
	return false;
      }

    if(::bind(m_sockfd,addr->getAddr(),addr->getAddrLen()) != 0)
      {
	LOG_ERROR(sys_log) << "bind error : " << strerror(errno)
			   << " errno : " << errno;
	return false;
      }
    m_localAddr = addr;
    return true;
  }
  
  bool Socket::listen(int backlog)
  {
    if(m_sockfd < 0)
      {
	LOG_ERROR(sys_log) << "failed to listen sockfd : sockfd = -1";
	return false;
      }
    int rt = ::listen(m_sockfd,backlog);
    if(rt)
      {
	LOG_ERROR(sys_log) << "listen error : "<<strerror(errno)
			   << " errno : "<<errno;
	return false;
      }
    return true;
  }

  bool Socket::close()
  {
    if(!m_connect || m_sockfd == -1)
      {
	return false;
      }
    if(m_sockfd != -1)
      {
	int rt = ::close(m_sockfd);
	if(rt < 0)
	  {
	    LOG_ERROR(sys_log) << "close("<<m_sockfd<<") error : "<<strerror(errno)
			       << " errno : " << errno;
	    return false;
	  }
	m_sockfd = -1;
	m_connect = false;
	return true;
      }
    return false;
  }

  Socket::ptr Socket::accept()
  {
    int fd = ::accept(m_sockfd,nullptr,nullptr);
    if(fd < 0)
      {
	if(errno !=ETIMEDOUT)
	  LOG_ERROR(sys_log) << "accept failed, error : "<<strerror(errno)
			     << " errno = "<<errno;
	return nullptr;
      }
    Socket::ptr sock(new Socket(m_family,m_type,m_protocol));
    if(sock->init(fd))
      return sock;
    return nullptr;
  }

  bool Socket::connect(IAddress::ptr addr)
  {
    m_remoteAddr = addr;
    if(m_connect) return false;
    if( m_sockfd == -1)
      {
	init();
	if(m_sockfd == -1) return false;
      }
    if(addr->getFamily() != m_family)
      {
	LOG_ERROR(sys_log) << "family type Substitute error , "
	  "required : "<<getFamilyType(m_family) << " actual : "
			   <<getFamilyType(addr->getFamily());
	return false;
      }
    if(::connect(m_sockfd,addr->getAddr(),addr->getAddrLen()) != 0)
      {
	LOG_ERROR(sys_log) << "connect failed,error : "<< strerror(errno)
			   << " errno = "<<errno;
	close();
	return false;
      }
    m_connect = 1;
    getRemoteAddr();
    getLocalAddr();
    return true;
  }

  bool Socket::reconnect()
  {
    if(!m_remoteAddr)
      {
	LOG_ERROR(sys_log) << "remote Address is null";
	return false;
      }
    m_localAddr.reset();
    return connect(m_remoteAddr);
    
  }
  std::ostream& Socket::dump(std::ostream& os)
  {
    os << "[ protocol : " << m_protocol 
       << " type : " << m_type 
       << " family : "<<m_family 
       << " connect : "<<m_connect 
       << " sockfd : "<<m_sockfd;
    if(m_localAddr)
      {
	os << " localAddr : "<<*m_localAddr;
      }
    if(m_remoteAddr)
      {
	os << " remoteAddr : "<<*m_remoteAddr;
      }
    os<<"]";
    return os;
  }

  int Socket::send(const void* buff,size_t length,int flags)
  {
    if(m_type != SOCK_STREAM) return -1;
    if(m_connect)
      {
	return ::send(m_sockfd,buff,length,flags);
      }
    return -1;
  }
  
  int Socket::send(const iovec* buff,size_t size,int flags)
  {
    if(m_type != SOCK_STREAM) return -1;
    if(m_connect)
      {
	struct msghdr msg;
	memset(&msg,0,sizeof(msg));
	msg.msg_iov = (iovec*)buff;
	msg.msg_iovlen = size;
	return ::sendmsg(m_sockfd,&msg,flags);
      }
    return -1;
  }
  
  int Socket::sendTo(const void* buff,size_t length,const IAddress::ptr to,int flags)
  {
    if(!to) return -1;
    if(m_type != SOCK_DGRAM)
      return -1;
    if(m_connect)
      {
	return ::sendto(m_sockfd,buff,length,flags,
			to->getAddr(),to->getAddrLen());
      }
    return -1;
  }
  
  int Socket::sendTo(const iovec* buff,size_t size,const IAddress::ptr to,int flags)
  {
    if(!to) return -1;
    if(m_type != SOCK_DGRAM) return -1;
    if(m_connect)
      {
	struct msghdr msg;
	memset(&msg,0,sizeof(msg));
	msg.msg_iov = (iovec*)buff;
	msg.msg_iovlen = size;
	msg.msg_name = to->getAddr();
	msg.msg_namelen = to->getAddrLen();
	return ::sendmsg(m_sockfd,&msg,flags);
      }
    return -1;
  }

  int Socket::recv(void* buff,size_t length,int flags)
  {
    if(m_type != SOCK_STREAM) return -1;
    if(m_connect)
      {
	return ::recv(m_sockfd,buff,length,flags);
      }
    return -1;
  }
  
  int Socket::recv(iovec* buff,size_t size,int flags)
  {
    if(m_type != SOCK_STREAM) return -1;
    if(m_connect)
      {
	struct msghdr msg;
	memset(&msg,0,sizeof(msg));
	msg.msg_iov = buff;
	msg.msg_iovlen = size;
	return ::recvmsg(m_sockfd,&msg,flags);
      }
    return -1;
  }
  
  int Socket::recvFrom(void* buff,size_t length,const IAddress::ptr from,int flags)
  {
    if(!from) return -1;
    if(m_type != SOCK_DGRAM)
      return -1;
    if(m_connect)
      {
	socklen_t addrlen =from->getAddrLen();
	return ::recvfrom(m_sockfd,buff,length,flags,
			  from->getAddr(),&addrlen);
      }
    return -1;
  }
  int Socket::recvFrom(iovec* buff,size_t size,const IAddress::ptr from,int flags)
  {
    if(!from) return -1;
    if(m_type != SOCK_DGRAM) return -1;
    if(m_connect)
      {
	struct msghdr msg;
	memset(&msg,0,sizeof(msg));
	msg.msg_iov = buff;
	msg.msg_iovlen = size;
	msg.msg_name = from->getAddr();
	msg.msg_namelen = from->getAddrLen();
	return ::recvmsg(m_sockfd,&msg,flags);
      }
    return -1;
  }

  //获取对端地址
  IAddress::ptr Socket::getRemoteAddr()
  {
    if(m_remoteAddr) return m_remoteAddr;
    IAddress::ptr addr;
    switch(m_family)
      {
      case  AF_INET :
	addr.reset(new IPv4Address());
	break;
      case AF_INET6 :
	addr.reset(new IPv6Address());
	break;
      case AF_UNIX :
	addr.reset(new UnixAddress());
	break;
      default :
	addr.reset(new UnknownAddress(m_family));
      }
    socklen_t addrlen = addr->getAddrLen();
    int rt = getpeername(m_sockfd,addr->getAddr(),&addrlen);
    if(rt < 0)
      {
	LOG_ERROR(sys_log) << "getpeername error : "<<strerror(errno)
			   << " errno : "<<errno;
	return nullptr;
      }
   //UnixAddress地址长度问题
    if(m_family == AF_UNIX)
      {
	UnixAddress::ptr tmp = std::dynamic_pointer_cast<UnixAddress>(addr);
	//TODO:调试信息,要删除
	ASSERT2(tmp!=nullptr,"tmp is null");
	if(tmp == nullptr)
	  {
	    std::stringstream ss;
	    ss << "UnixAddress : "<< addr <<" is not valid";
	    throw std::logic_error(ss.str());
	  }
	  tmp->setAddrLen(addrlen);
      }
    m_remoteAddr = addr;
    return m_remoteAddr;
  }
  IAddress::ptr Socket::getLocalAddr()
  {
    if(m_localAddr) return m_localAddr;
    IAddress::ptr addr;

    switch(m_family)
      {
      case AF_INET :
	addr.reset(new IPv4Address());
	break;
      case AF_INET6 :
	addr.reset(new IPv6Address());
	break;
      case AF_UNIX:
	addr.reset(new UnixAddress());
	break;
      default :
	addr.reset(new UnknownAddress(m_family));
      }
    socklen_t addrlen = addr->getAddrLen();
    int rt = getsockname(m_sockfd,addr->getAddr(),&addrlen);
    if(rt < 0)
      {
	LOG_ERROR(sys_log) << "getsockname error : "<<strerror(errno)
			   << " errno : "<<errno;
	return nullptr;
      }
    if(m_family == AF_UNIX)
      {
	UnixAddress::ptr tmp = std::dynamic_pointer_cast<UnixAddress>(addr);
	//TODO:调试信息,要删除
	ASSERT2(tmp!=nullptr,"tmp is null");
	if(tmp == nullptr)
	  {
	    std::stringstream ss;
	    ss << "UnixAddress : "<< addr <<" is not valid";
	    throw std::logic_error(ss.str());
	  }
	tmp->setAddrLen(addrlen);
      }
    m_localAddr = addr;
    return m_localAddr;
  }

  Socket::ptr Socket::newTCPSocket()
  {
    Socket::ptr sock(new Socket(AF_INET,SOCK_STREAM,0));
    return sock;
  }
  
  Socket::ptr Socket::newTCPSocketv6()
  {
    Socket::ptr sock( new Socket(AF_INET6,SOCK_STREAM,0));
    return sock;
  }
  
  Socket::ptr Socket::newUDPSocket()
  {
    Socket::ptr sock(new Socket(AF_INET,SOCK_DGRAM,0));
    return sock;
  }
  
  Socket::ptr Socket::newUDPSocketv6()
  {
    Socket::ptr sock(new Socket(AF_INET6,SOCK_DGRAM,0));
    return sock;
  }
  
  Socket::ptr Socket::newUnixTCPSocket()
  {
    Socket::ptr sock(new Socket(AF_UNIX,SOCK_STREAM,0));
    return sock;
  }

  Socket::ptr Socket::newUnixUDPSocket()
  {
    Socket::ptr sock(new Socket(AF_UNIX,SOCK_DGRAM,0));
    return sock;
  }

};
