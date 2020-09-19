#pragma once

#include"socket/TcpServer.hpp"
#include"HttpSession.hpp"
#include<memory>
#include"scheduler/Scheduler.hpp"
#include"Servlet.hpp"

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
    ServletDispatch::ptr getServletDispatch() const
    {
      return m_dispatch;
    }
    void setServletDispatch(ServletDispatch::ptr dispatch)
    {
      m_dispatch = dispatch;
    }
  protected:
    virtual void handleClient(Socket::ptr client) override;
  private:
    bool m_keepAlive;
    ServletDispatch::ptr m_dispatch;
  };
  
}
}
