#include"WorkServlet.hpp"
#include"../http/HttpConnection.hpp"

namespace zhuyh
{
namespace proxy
{
  
  int32_t ProxyServlet::handle(http::HttpRequest::ptr req,
			       http::HttpResponse::ptr& resp,
			       http::HttpSession::ptr session)
  {
    std::string host = req->getHeader("Host","");
    auto scheme = req->getScheme();
    //    LOG_ROOT_ERROR() << "Host : " << host;

    if(strcasecmp(req->getHeader("connection","close").c_str(), "keep-alive") == 0)
      req->setClose(false);
    if(host.empty())
      {
	return std::make_shared<http::NotFoundServlet>()->handle(req,resp,session);
      }
    Socket::ptr sock = Socket::newTCPSocket();
    IPAddress::ptr addr = IAddress::newAddressByHostAnyIp(host);
    if(addr == nullptr)
      {
	return std::make_shared<http::NotFoundServlet>()->handle(req,resp,session);
      }
    //默认连接80端口
    if(addr->getPort() == 0) addr->setPort(80);
    int rt = sock->connect(addr);
    if(rt == false)
      {
	return std::make_shared<http::NotFoundServlet>()->handle(req,resp,session);
      }
    http::HttpConnection::ptr conn = std::make_shared<http::HttpConnection>(sock);
    if(conn->sendRequest(req) < 0)
      {
	return std::make_shared<http::NotFoundServlet>()->handle(req,resp,session);
      }
    rt  = conn->recvResponse(resp);
    if(rt == false)
      {
	return std::make_shared<http::NotFoundServlet>()->handle(req,resp,session);
      }
    LOG_ROOT_INFO()<<"\r\n"<<*req<<std::endl;
    LOG_ROOT_INFO()<<"\r\n"<<*resp<<std::endl;
    return 0;
  }

  int32_t ConnectServlet::handle(http::HttpRequest::ptr req,
				 http::HttpResponse::ptr& resp,
				 http::HttpSession::ptr session)
  {
    resp->setStatus(http::HttpStatus::OK);
    resp->setBody(req->toString());
    return 0;
  }
}
}
