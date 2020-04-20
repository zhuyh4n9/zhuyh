#include "Servlet.hpp"
#include <fnmatch.h>

namespace zhuyh
{
namespace http
{

  ServletDispatch::ServletDispatch(Servlet::ptr dft ,const std::string& name)
      :Servlet(name),
       m_dft(dft)
    {
      if(m_dft == nullptr)
	m_dft.reset(new NotFoundServlet("httpServer/1.0.0"));
    }
  int32_t ServletDispatch::handle(HttpRequest::ptr req,
				  HttpResponse::ptr& resp,
				  HttpSession::ptr session)
  {
    auto slt = getMatchServlet(req->getPath());
    if(slt)
      {
	slt->handle(req,resp,session);
      }
    return 0;
  }

  std::vector<std::pair<std::string,Servlet::ptr>>
  ServletDispatch::listAllServlet()
  {
    std::vector<std::pair<std::string,Servlet::ptr>> res;
    for(auto& item : m_datas)
      {
	res.push_back(item);
      }
    return res;
  }
  std::vector<std::pair<std::string,Servlet::ptr>>
  ServletDispatch::listAllGlobServlet()
  {
    std::vector<std::pair<std::string,Servlet::ptr>> res;
    for(auto& item : m_globs)
      {
	res.push_back(item);
      }
    return res;
  }
  void ServletDispatch::addServlet(const std::string& uri,Servlet::ptr slt)
  {
    WRLockGuard lg(m_lk);
    m_datas[uri] = slt;
  }
  void ServletDispatch::addServlet(const std::string& uri,
				   FunctionServlet::CbType cb)
  {
    WRLockGuard lg(m_lk);
    m_datas[uri].reset(new FunctionServlet(cb));
  }

  void ServletDispatch::addGlobServlet(const std::string& uri,Servlet::ptr slt)
  {
    WRLockGuard lg(m_lk);
    for(auto it = m_globs.begin();it!=m_globs.end();it++)
      {
	if(it->first == uri)
	  {
	    m_globs.erase(it);
	    break;
	  }
      }
    m_globs.push_back(std::make_pair(uri,slt));
  }
  void ServletDispatch::addGlobServlet(const std::string& uri,
				       FunctionServlet::CbType cb)
  {
    addGlobServlet(uri,std::make_shared<FunctionServlet>(cb));
  }
  
  void ServletDispatch::delServlet(const std::string& uri)
  {
    WRLockGuard lg(m_lk);
    m_datas.erase(uri);
  }
  void ServletDispatch::delGlobServlet(const std::string& uri)
  {
    WRLockGuard lg(m_lk);
    for(auto it = m_globs.begin();it!=m_globs.end();it++)
      {
	if(it->first == uri)
	  {
	    m_globs.erase(it);
	    break;
	  }
      }
  }
  
  Servlet::ptr ServletDispatch::getServlet(const std::string& uri)
  {
    RDLockGuard lg(m_lk);
    auto it = m_datas.find(uri);
    return it == m_datas.end() ? nullptr : it->second;
  }
  Servlet::ptr ServletDispatch::getGlobServlet(const std::string& uri)
  {
    WRLockGuard lg(m_lk);
    for(auto it = m_globs.begin();it!=m_globs.end();it++)
      {
	if(it->first == uri)
	  {
	    return it->second;
	  }
      }
    return nullptr;
  }
  
  Servlet::ptr ServletDispatch::getMatchServlet(const std::string& uri)
  {
    //LOG_ROOT_ERROR() << uri;
    RDLockGuard lg(m_lk);
    auto it = m_datas.find(uri);
    if(it != m_datas.end())
      {
	//LOG_ROOT_INFO() << "HERE1";
	return it->second;
      }
    for(auto it = m_globs.begin();it!=m_globs.end();it++)
      {
	if(fnmatch(it->first.c_str(), uri.c_str(),0) == 0)
	  {
	    //LOG_ROOT_INFO() << "HERE2";
	    return it->second;
	  }
      }
    //LOG_ROOT_INFO() << "HERE3";
    return m_dft;
  }

  int32_t NotFoundServlet::handle(HttpRequest::ptr req,
				  HttpResponse::ptr& resp,
				  HttpSession::ptr session)
  {
    resp->setStatus(HttpStatus::NOT_FOUND);
    resp->setHeader("Server","HttpServer/1.0.0");
    resp->setHeader("Content-Type","text/html");
    resp->setBody(m_content);
    return 0;
  }

  int32_t MethodNotAllowedServlet::handle(HttpRequest::ptr req,
					  HttpResponse::ptr& resp,
					  HttpSession::ptr session)
  {
    resp->setStatus(HttpStatus::METHOD_NOT_ALLOWED);
    resp->setHeader("Server","HttpServer/1.0.0");
    resp->setHeader("Content-Type","text/html");
    resp->setBody(m_content);
    return 0;
  }
  
}
}
