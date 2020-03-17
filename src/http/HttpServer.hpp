#pragma once

#include"../socket/TcpServer.hpp"
#include"HttpSession.hpp"
#include<memory>
#include"../scheduler/Scheduler.hpp"

namespace zhuyh
{
namespace http
{
  class HttpServer : public TcpServer
  {
  public:
    typedef std::shared_ptr<HttpServer> ptr;
    HttpServer(bool keepAlive = false,Scheduler* schd = nullptr,
	       Scheduler* accept_schd = nullptr,
	       const std::string& name = "HttpServer/1.0.0");
  protected:
    virtual void handleClient(Socket::ptr client) override;
  private:
    bool m_keepAlive;
  };
  
}
}
