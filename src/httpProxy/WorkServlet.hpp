#pragma once

#include"../logs.hpp"
#include"../http/Http.hpp"
#include"../http/HttpSession.hpp"
#include"../http/Servlet.hpp"
#include<fstream>
#include"../db/mysql.hpp"

namespace zhuyh
{
namespace proxy
{
  class ProxyServlet : public http::Servlet
  {
  public:
    typedef std::shared_ptr<ProxyServlet> ptr;
    ProxyServlet()
      :http::Servlet("ProxyServlet"){}
    int32_t handle(http::HttpRequest::ptr req,
		   http::HttpResponse::ptr& resp,
		   http::HttpSession::ptr session) override;
  };

  class ConnectServlet : public http::Servlet
  {
  public:
    typedef std::shared_ptr<ConnectServlet> ptr;
    ConnectServlet()
      :http::Servlet("ConnectServlet"){}
    int32_t handle(http::HttpRequest::ptr req,
		   http::HttpResponse::ptr& resp,
		   http::HttpSession::ptr session) override;
  };
}
}
