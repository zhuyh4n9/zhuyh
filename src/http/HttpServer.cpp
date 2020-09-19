#include "HttpServer.hpp"
#include "logs.hpp"

namespace zhuyh
{
namespace http
{
  static Logger::ptr s_logger = GET_LOGGER("system");
  HttpServer::HttpServer(bool keepAlive ,Scheduler* schd,
			 Scheduler* accept_schd,
			 const std::string& name)
    :TcpServer(schd,accept_schd,name),
     m_keepAlive(keepAlive),
     m_dispatch(std::make_shared<ServletDispatch>())
  {
  }

  void HttpServer::handleClient(Socket::ptr client)
  {
    HttpSession::ptr session = std::make_shared<HttpSession>(client);
    do
      {
	HttpRequest::ptr req = session->recvRequest();
	if(!req)
	  {
	    if(!errno ) break;
	    LOG_WARN(s_logger) << "receive request failed, error : "<<strerror(errno)
			       <<" errno : "<<errno<<" client : "<<client;
	    break;
	  }
	// std::cout<<*req<<std::endl;
	// std::cout<<"uri : "<<req->getUri()<<std::endl;
	// std::cout<<"scheme :"<<req->getScheme()<<std::endl;
	// std::cout<<"Path :"<<req->getPath()<<std::endl;
	//LOG_ROOT_INFO() <<*req;
	HttpResponse::ptr resp =
	  std::make_shared<HttpResponse>(req->getVersion(),
					 req->isClose()||!m_keepAlive);
	m_dispatch->handle(req,resp,session);
	//LOG_ROOT_INFO()<<*resp;
	session->sendResponse(resp);
      }
    while(m_keepAlive);
  }
  
}
}
