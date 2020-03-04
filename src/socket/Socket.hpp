#pragma once

#include<unistd.h>
#include<memory>
#include<sys/socket.h>
#include<sys/types.h>
#include<cstring>
#include<sys/un.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include <vector>
#include <iostream>
#include "Address.hpp"
#include <iostream>
#include <sstream>
#include "../logs.hpp"
#include "../macro.hpp"
namespace zhuyh
{
  static Logger::ptr sys_log = GET_LOGGER("system");
  class Socket : public std::enable_shared_from_this<Socket>
  {
  public:
    //获取m_family类型
    std::string getFamilyType(int family)
    {
#define XX(Family,Target)			\
      if(Family == Target)			\
	return #Target;
      XX(family,AF_INET);
      XX(family,AF_INET6);
      XX(family,AF_UNIX);
      XX(family,AF_UNSPEC);
#undef XX
      return "UNKNOWN";
    }
  public:
    typedef std::shared_ptr<Socket> ptr;
    typedef std::weak_ptr<Socket> wptr;
  private:
    //禁止默认拷贝构造函数和赋值函数
    Socket(const Socket& sock) = delete;
    Socket& operator= (const Socket& sock) = delete;
  public:
    //创建各种Socket
    static Socket::ptr newTCPSocket();
    static Socket::ptr newTCPSocketv6();
    static Socket::ptr newUDPSocket();
    static Socket::ptr newUDPSocketv6();
    static Socket::ptr newUnixTCPSocket();
    static Socket::ptr newUnixUDPSocket();
  public:
    Socket(int family,int type,int protocol);
    ~Socket();
    //输出TCP相关信息
    std::ostream& dump(std::ostream& os);
    //获取/设置发送/接收超时,单位(ms)
    uint64_t getSendTimeout() const ;
    bool setSendTimeout(uint64_t to);
    uint64_t getRecvTimeout() const;
    bool setRecvTimeout(uint64_t to);

    //创建一个socket,并且初始化
    bool init(int sockfd);
    bool bind(IAddress::ptr addr);
    bool listen(int backlog = SOMAXCONN);
    bool close();
    Socket::ptr accept();
    bool connect(IAddress::ptr addr);
    bool reconnect();
    int send(const void* buff,size_t length,int flags = 0);
    int send(const iovec* buff,size_t size,int flags = 0);
    int sendTo(const void* buff,size_t length,const IAddress::ptr to,int flags = 0);
    int sendTo(const iovec* buff,size_t size,const IAddress::ptr to,int flags = 0);

    int recv(void* buff,size_t length,int flags = 0);
    int recv(iovec* buff,size_t size,int flags = 0);
    int recvFrom(void* buff,size_t length,const IAddress::ptr from,int flags = 0);
    int recvFrom(iovec* buff,size_t size,const IAddress::ptr from,int flags = 0);
    
    IAddress::ptr getRemoteAddr();
    IAddress::ptr getLocalAddr();
    int getFamily() const
    {
      return m_family;
    }
    int getType() const
    {
      return m_type;
    }
    int getProtocol() const
    {
      return m_protocol;
    }

    bool isConnect() const
    {
      return m_connect;
    }
    bool isValid() const
    {
      return m_sockfd != -1;
    }
    int getError() const;
    int getSockFd() const
    {
      return m_sockfd;
    }
    std::string toString()
    {
      std::stringstream ss;
      dump(ss);
      return ss.str();
    }
  private:
    //设置地址复用,取消Nagle算法
    bool initSock();
    //创建一个套接字描述符,并且初始化
    bool init();
  public:
    template<typename T>
    bool setSockOption(int level, int optname,const T& val)
    {
      if(m_sockfd < 0 ) return false;
      return setSockOption(level,optname,(void*)&val,sizeof(T));
    }
    template<typename T>
    bool getSockOption(int level, int optname,T& res)
    {
      if( m_sockfd < 0 ) return false;
      socklen_t len = sizeof(T);
      return getSockOption(level,optname,(void*)&res,&len);
    }
  private:
    bool setSockOption(int level, int optname,
		       const void *optval, socklen_t optlen);
    bool getSockOption(int level, int optname,
		       void *res, socklen_t* optlen);
  private:
    int m_sockfd = -1;
    int m_family;
    int m_protocol;
    int m_type;
    bool m_connect = false;
    IAddress::ptr m_localAddr = nullptr;
    IAddress::ptr m_remoteAddr = nullptr;
  };
  
  inline std::ostream& operator<<(std::ostream& os,const Socket::ptr& sock)
  {
    sock->dump(os);
    return os;
  }
}
