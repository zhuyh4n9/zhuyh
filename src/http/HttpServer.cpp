#include "HttpServer.hpp"
#include "../logs.hpp"

namespace zhuyh
{
namespace http
{
  static Logger::ptr s_logger = GET_LOGGER("system");
  HttpServer::HttpServer(bool keepAlive ,Scheduler* schd,
			 Scheduler* accept_schd,
			 const std::string& name)
    :TcpServer(schd,accept_schd,name),
     m_keepAlive(keepAlive)
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
	    LOG_WARN(s_logger) << "receive request failed, error : "<<strerror(errno)
			       <<" errno : "<<errno<<" client : "<<client;
	    break;
	  }
	HttpResponse::ptr resp =
	  std::make_shared<HttpResponse>(req->getVersion(),
					 req->isClose()||!m_keepAlive);
	resp->setBody("Hello World");
	int rt = session->sendResponse(resp);
	if(rt <= 0)
	  {
	    LOG_WARN(s_logger) << "send response failed , error : "<<strerror(errno)
			       <<" errno : "<<errno<<" client : "<<client;
	    break;
	  }
      }
    while(m_keepAlive);
  }
  
}
}
