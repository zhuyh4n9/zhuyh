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
  class IndexServlet : public http::Servlet
  {
  public:
    typedef std::shared_ptr<IndexServlet> ptr;
    IndexServlet(const std::string& path)
      :Servlet("MainServlet"),
       m_path(path){}
    int32_t handle(http::HttpRequest::ptr req,
		   http::HttpResponse::ptr& resp,
		   http::HttpSession::ptr session) override;
  private:
    std::string m_path;
  };

  class MateriaServlet : public http::Servlet
  {
    public:
    typedef std::shared_ptr<MateriaServlet> ptr;
    MateriaServlet(const std::string& path)
      :Servlet("MateriaServlet"),
       m_path(path){}
    int32_t handle(http::HttpRequest::ptr req,
		   http::HttpResponse::ptr& resp,
		   http::HttpSession::ptr session) override;
  private:
    std::string m_path;
  };

  class LogInServlet : public http::Servlet
  {
  public:
    typedef std::shared_ptr<LogInServlet> ptr;
    LogInServlet(const std::string& path)
      :Servlet("LogInServlet"),
       m_path(path)
    {
      auto conn = db::MySQLConn::Create("root");
      db::MySQLConnGuard cg(conn);
      std::string sql = "SELECT 1 FROM Account WHERE username = ? and passwd = ? LIMIT 0,1";
      m_stmt = db::MySQLStmt::Create(conn,sql);
      ASSERT(m_stmt != nullptr);
    }
    int32_t handle(http::HttpRequest::ptr req,
		   http::HttpResponse::ptr& resp,
		   http::HttpSession::ptr session) override;
  private:
    db::MySQLStmt::ptr m_stmt;
    std::string m_path;
  };

  class RegisterServlet : public http::Servlet
  {
  public:
    typedef std::shared_ptr<RegisterServlet> ptr;
    RegisterServlet(const std::string& path)
      :Servlet("RegisterServlet"),
       m_path(path)
    {
      auto conn = db::MySQLConn::Create("root");
      //db::MySQLConnGuard cg(conn);
      std::string sql = "INSERT INTO Account(username,email,passwd) Values(?,?,?)";
      m_stmt = db::MySQLStmt::Create(conn,sql);
      ASSERT(m_stmt != nullptr);
    }

    int32_t handle(http::HttpRequest::ptr req,
		   http::HttpResponse::ptr& resp,
		   http::HttpSession::ptr session) override;
  private:
    db::MySQLStmt::ptr m_stmt;
    std::string m_path;
  };
}
}
