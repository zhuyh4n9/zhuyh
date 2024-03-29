#include "TcpServer.hpp"
#include "config/config.hpp"
#include "logs.hpp"
#include <string.h>
#include <errno.h>

namespace zhuyh
{
  static Logger::ptr s_logger = GET_LOGGER("system");
  static ConfigVar<uint64_t>::ptr s_tcp_server_timeout =
    Config::lookUp<uint64_t>("tcp_server.read_timeout",
			     (uint64_t)60*1000,"tcp server read timeout");
  
  TcpServer::TcpServer(Scheduler* schd,
		       Scheduler* accept_schd,
		       const std::string& name)
    :m_schd(schd),
     m_acceptSchd(accept_schd),
     m_recvTimeout(s_tcp_server_timeout->getVar()),
     m_name(name),
     m_stop(true)
  {
    if(m_schd == nullptr)
      {
	//默认使用根调度器
	m_schd = Scheduler::Schd::getInstance();
      }
    if(m_acceptSchd == nullptr)
      {
	//默认使用根调度器
	m_acceptSchd = Scheduler::Schd::getInstance();
      }
    //    LOG_ROOT_INFO() << "HERE";
    m_schd->start();
    m_acceptSchd->start();
  }
  Socket::ptr TcpServer::bind(uint32_t family)
  {
    Socket::ptr sock = nullptr;
    switch(family)
      {
      case AF_INET:
	sock = Socket::newTCPSocket();
	break;
      case AF_INET6:
	sock = Socket::newTCPSocketv6();
	break;
      case AF_UNIX:
	sock = Socket::newUnixTCPSocket();
	break;
      default:
	return nullptr;
      }
    return sock;
  }
  
  TcpServer::~TcpServer()
  {
    for(auto& item: m_socks)
      item->close();
    m_socks.clear();
  }
  bool TcpServer::bind(IAddress::ptr address)
  {
    Socket::ptr sock = bind(address->getFamily());
    if(!sock)
      { 
	LOG_ERROR(s_logger) << "Failed to bind address : ["<<*address<<"] errno :"<<errno
			    <<" error : "<<strerror(errno);
	return false;
      }
    if(!sock->bind(address))
      return false;
    if(!sock->listen())
      return false;
    if(!sock->setRecvTimeout(m_recvTimeout))
      return false;
    m_socks.push_back(sock);
    LOG_INFO(s_logger) << "server bind success : "<<sock;
    return true;
  }
  
  bool TcpServer::bind(const std::vector<IAddress::ptr>& address,
		       std::vector<IAddress::ptr>& failed)
  {
    int rt = true;
    for(auto& item : address)
      {
	Socket::ptr sock = bind(item->getFamily());
	if(!sock)
	  {
	    rt = false;
	    failed.push_back(item);
	    LOG_ERROR(s_logger) << "Failed to bind address : ["<<*item<<"] errno :"<<errno
				<<" error : "<<strerror(errno);
	    continue;
	  }
	if(!sock->bind(item))
	  {
	    rt = false;
	    failed.push_back(item);
	    continue;
	  }
	if(!sock->listen())
	  {
	    rt =  false;
	    failed.push_back(item);
	    continue;
	  }
	if(!sock->setRecvTimeout(m_recvTimeout))
	  {
	    rt = false;
	    failed.push_back(item);
	    LOG_ERROR(s_logger) << "Failed to set Recv timeout";
	    continue;
	  }
	m_socks.push_back(sock);
      }
    if(!rt)
	m_socks.clear();
    else
      for(auto& item : m_socks)
	{
	  LOG_ERROR(s_logger) << "server bind success : "<<item;
	}
    return rt;
  }
  
  void TcpServer::handleClient(Socket::ptr client)
  {
    LOG_ROOT_INFO() << "handle client : "<<client;
  }
  void TcpServer::startAccept(Socket::ptr sock)
  {
    //LOG_ROOT_INFO() << "start accept : "<<sock->getSockFd();
    while(!m_stop)
      {
	Socket::ptr client = sock->accept();
	if(client)
	  {
	    //LOG_ROOT_ERROR() << client;
	    client->setRecvTimeout(m_recvTimeout);
	    //shared_from_this防止TcpServer再处理时被释放
	    m_schd->addNewFiber(std::bind(&TcpServer::handleClient,
					 shared_from_this(),client));
	  }
	else if(errno != ETIMEDOUT)
	  {
	    LOG_ERROR(s_logger) << "accept failed , errno : "<<errno
				<< " error : "<<strerror(errno);
	  }
	else
	  {
	    LOG_ERROR(s_logger) << "accept timedout , errno : "<<errno
				<< " error : "<<strerror(errno);
	  }
      }
  }
  //启动accept
  bool TcpServer::start()
  {
    if(!m_stop) return true;
    m_stop = false;
    for(auto& sock:m_socks)
      m_acceptSchd->addNewFiber(std::bind(&TcpServer::startAccept,
					 shared_from_this(),sock));
    return true;
  }
  bool TcpServer::stop()
  {
    m_stop = true;
    auto self = shared_from_this();
    m_acceptSchd->addNewFiber([this,self](){
         auto schd = Scheduler::getThis();
         for(auto& sock : m_socks)
	   {
	     schd->cancelAllEvent(sock->getSockFd());
	     sock->close();
	   }
    });
    m_socks.clear();
    return true;
  }

}
