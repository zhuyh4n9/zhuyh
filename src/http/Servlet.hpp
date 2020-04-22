#pragma once

#include <memory>
#include <functional>
#include <string>
#include "Http.hpp"
#include "HttpSession.hpp"
#include <vector>
#include <unordered_map>
#include "../latch/lock.hpp"

namespace zhuyh
{
namespace http
{


  class Servlet
  {
  public:
    typedef std::shared_ptr<Servlet> ptr;
    Servlet(const std::string& name)
      :m_name(name)
    {}
    virtual ~Servlet() {}
    virtual int32_t handle(HttpRequest::ptr req,
			   HttpResponse::ptr& resp,
			   HttpSession::ptr session) = 0;
    const std::string& getName() const
    {
      return m_name;
    }
  private:
    std::string m_name;
  };
  

  
  class MethodNotAllowedServlet : public Servlet
  {
  public :
    typedef std::shared_ptr<MethodNotAllowedServlet> ptr;
    MethodNotAllowedServlet(const std::string& name = "HttpServer/1.0.0")
      :Servlet("MethodNotAllowedServlet")
    {
      m_content = "<html><head><title>405 Method Not Allowed"
        "</title></head><body><center><h1>405 Method Not Allowed</h1></center>"
        "<hr><center>" + name + "</center></body></html>";
    }
    virtual int32_t handle(HttpRequest::ptr req,
			   HttpResponse::ptr& resp,
			   HttpSession::ptr session) override;
  private:
    std::string m_content;
  };
  
  class MethodServlet : public Servlet
  {
  public:
    typedef std::shared_ptr<MethodServlet> ptr;
    MethodServlet(const std::string& name)
      :Servlet(name),
       m_dft(new MethodNotAllowedServlet())
    {
    }

    virtual int32_t handle(HttpRequest::ptr req,
			   HttpResponse::ptr& resp,
			   HttpSession::ptr session) override
    {
      HttpMethod method = req->getMethod();
      auto it = m_servlets.find((uint32_t)method);
      if(it == m_servlets.end())
	{
	  return m_dft->handle(req,resp,session);
	}
      return it->second->handle(req,resp,session);
    }

    void addServlet(HttpMethod method,Servlet::ptr servlet)
    {
      m_servlets[(uint32_t)method] = servlet;
    }
    void setDefault(Servlet::ptr servlet)
    {
      m_dft = servlet;
    }
  private:
    std::map<uint32_t,Servlet::ptr> m_servlets;
    Servlet::ptr m_dft;
  };


  class FunctionServlet : public Servlet
  {
  public:
    typedef std::shared_ptr<FunctionServlet> ptr;
    typedef std::function<int32_t(HttpRequest::ptr,
				  HttpResponse::ptr,
				  HttpSession::ptr)> CbType;
    FunctionServlet(CbType cb,
		    const std::string& name = "FunctionServlet")
      :Servlet(name),
       m_cb(cb)
    {
    }
    virtual int32_t handle(HttpRequest::ptr req,
			   HttpResponse::ptr& resp,
			   HttpSession::ptr session) override
    {
      return m_cb(req,resp,session);
    }
  private:
    CbType m_cb;
  };
  
  class ServletDispatch : public Servlet
  {
  public:
    typedef std::shared_ptr<ServletDispatch> ptr;

    ServletDispatch(Servlet::ptr dft = nullptr,const std::string& name = "ServletDispatch");
    
    virtual int32_t handle(HttpRequest::ptr req,
			   HttpResponse::ptr& resp,
			   HttpSession::ptr session) override;
    
    void addServlet(const std::string& uri,Servlet::ptr slt);
    void addServlet(const std::string& uri,FunctionServlet::CbType slt);

    void addGlobServlet(const std::string& uri,Servlet::ptr slt);
    void addGlobServlet(const std::string& uri,FunctionServlet::CbType slt);

    void delServlet(const std::string& uri);
    void delGlobServlet(const std::string& uri);

    Servlet::ptr getServlet(const std::string& uri);
    Servlet::ptr getGlobServlet(const std::string& uri);
    
    std::vector<std::pair<std::string,Servlet::ptr>> listAllServlet();
    std::vector<std::pair<std::string,Servlet::ptr>> listAllGlobServlet();
    //优先访问m_data
    //其次m_globs;
    //最后返回dft
    Servlet::ptr getMatchServlet(const std::string& uri);

    Servlet::ptr getDefault() const
    {
      return m_dft;
    }
  private:
    std::unordered_map<std::string,Servlet::ptr> m_datas;
    std::vector<std::pair<std::string,Servlet::ptr>> m_globs;
    //默认Servlet
    Servlet::ptr m_dft;
    RWLock m_lk;
  };

  class NotFoundServlet : public Servlet
  {
  public :
    typedef std::shared_ptr<NotFoundServlet> ptr;
    NotFoundServlet(const std::string& name = "HttpServer/1.0.0")
      :Servlet("NotFoundServlet")
    {
      m_content = "<html><head><title>404 Not Found"
        "</title></head><body><center><h1>404 Not Found</h1></center>"
        "<hr><center>" + name + "</center></body></html>";
    }
    virtual int32_t handle(HttpRequest::ptr req,
			   HttpResponse::ptr& resp,
			   HttpSession::ptr session) override;
  private:
    std::string m_content;
  };

}
}
