#include"HttpProxyServer.hpp"
#include"../scheduler/Scheduler.hpp"
#include"../macro.hpp"
#include"../socket/Address.hpp"

namespace zhuyh
{
namespace proxy
{
  Scheduler::ptr dns_schd;
  namespace{
    struct DNS_INITER{
      DNS_INITER()
      {
	dns_schd.reset(new Scheduler("dns_schd",4));
	dns_schd->start();
      }
    };
    DNS_INITER __dns_initer;
  }
  
  http::HttpSession::ptr HttpProxyServer::makeSession(Socket::ptr sock)
  {
    auto session = std::make_shared<http::HttpSession>(sock);
    return session;
  }
  http::HttpConnection::ptr HttpProxyServer::makeConnection(const std::string& host,uint16_t dft)
  {
    auto sock = Socket::newTCPSocket();
    IPAddress::ptr addr;
    auto schd = Scheduler::getThis();
    auto fb = Fiber::getThis();
    addr = IAddress::newAddressByHostAnyIp(host);
    dns_schd->addNewTask([schd,fb,&addr,&host]() {
    			   addr = IAddress::newAddressByHostAnyIp(host);
    			   schd->addNewTask(fb);
    			 });
    zhuyh::Fiber::YieldToHold();
    if(addr == nullptr)
      {
	LOG_ROOT_ERROR() << "host : " << host << " is not valid";
	return nullptr;
      }
    if(addr->getPort() == 0) addr->setPort(dft);
    if(sock->connect(addr) == false) return nullptr;
    auto connection = std::make_shared<http::HttpConnection>(sock);
    return connection;
  }

  void HttpProxyServer::getDNS(Scheduler* schd,Fiber::ptr fb,
			       IAddress::ptr& addr,const std::string& host)
  {

  }
  
  void HttpProxyServer::handleClient(Socket::ptr client)
  {
    auto session = makeSession(client);
    auto req = session->recvRequest();
    if(req == nullptr) return handleNotFound(session);
    auto keep = req->getHeader("Connection","close");
    if(strncasecmp(keep.c_str(),"keep-alive",keep.size()) == 0)
      req->setClose(false);
    
    std::string host = req->getHeader("Host","");
    if(host.empty()) return handleNotFound(session);
    auto connection = makeConnection(host);
    if(connection == nullptr) return handleNotFound(session,req->toString());
    connection->sendRequest(req);

    http::HttpResponse::ptr resp = connection->recvResponse();
    if(resp == nullptr) return handleNotFound(session);

    int rt = session->sendResponse(resp);
    if(rt < 0)
      {
	LOG_ROOT_ERROR() << "Failed to send response, error : "<<strerror(errno)
			 <<",errno : "<<errno;
      }

    
  }

  void HttpProxyServer::handleNotFound(http::HttpSession::ptr session,const std::string& msg)
  {
    auto resp = std::make_shared<http::HttpResponse>();
    
    resp->setStatus(http::HttpStatus::NOT_FOUND);
    auto content = "<html><head><title>404 Not Found"
      "</title></head><body><center><h1>404 Not Found</h1></center>"
      "<hr><center>"+msg+"</center></body></html>";
    resp->setBody(content);
    int rt = session->sendResponse(resp);
    if(rt < 0)
      {
	LOG_ROOT_ERROR() << "Failed to send response, error : "<<strerror(errno)
			 <<",errno : "<<errno;
      }
  }
  
}
}
