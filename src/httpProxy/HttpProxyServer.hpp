#pragma once

#include"LogInServlet.hpp"
#include"../config.hpp"
#include<string>
#include"../http/HttpServer.hpp"
#include"../co.hpp"
#include"../http/HttpConnection.hpp"
#include"../http/HttpSession.hpp"

namespace zhuyh
{
namespace proxy
{
  class HttpProxyServer : public TcpServer
  {
  public:
    HttpProxyServer(Scheduler* schd = nullptr,
		    Scheduler* accept_schd = nullptr,
		    const std::string& name = "HttpProxyServer/1.0.0")
      :TcpServer(schd,accept_schd,name){
    }
    void handleClient(Socket::ptr client) override;
  protected:
    static void getDNS(Scheduler* schd,Fiber::ptr fb,IAddress::ptr& addr,const std::string& host);
    http::HttpSession::ptr makeSession(Socket::ptr sock);
    http::HttpConnection::ptr makeConnection(const std::string& host,uint16_t dft = 80);
    void handleNotFound(http::HttpSession::ptr session,
			const std::string& msg = "HttpProxyServer");
  };
}
}
