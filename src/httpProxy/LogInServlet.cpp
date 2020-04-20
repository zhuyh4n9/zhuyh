#include"LogInServlet.hpp"

namespace zhuyh
{
namespace proxy
{

  static bool hasValue(const std::string& key,
		       std::unordered_map<std::string,std::string>& mp,
		       std::string& value)
  {
    auto it = mp.find(key);
    if(it == mp.end() ) return false;
    value = it->second;
    return true;
  }
  int32_t IndexServlet::handle(http::HttpRequest::ptr req,
			      http::HttpResponse::ptr resp,
			      http::HttpSession::ptr session)
  {
    std::string content;
    std::ifstream fin(m_path,std::ios::binary | std::ios::ate);
    if(!fin)
      {
	resp->setStatus(http::HttpStatus::NOT_FOUND);
	content = "<html><head><title>404 Not Found"
	  "</title></head><body><center><h1>404 Not Found</h1></center>"
	  "<hr><center>Index.html NOT FOUND</center></body></html>";
	resp->setBody(content);
	return 0;
      }
    
    auto remain = fin.tellg();
    auto off = 0;
    // std::cout<<"file size : "<<remain<<std::endl
    // 	     <<"path : "  << m_path<<std::endl;
    content.resize(remain);
    while(!fin.eof() && remain != 0)
      {
	int nread = 0;
	fin.seekg(0);
	try
	  {
	    if(fin.read(&content[off],remain))
	      {
		nread = fin.gcount();
		remain -= nread;
		off+=nread;
	      }
	    else
	      {
		LOG_ROOT_ERROR() << "failed to read file <"<<m_path<<">";
		break;
	      }
	  }
	catch(std::exception& e)
	  {
	    LOG_ROOT_ERROR() << "failed to read file <" <<m_path<<">"
			     <<" error : "<<e.what();
	    break;
	  }
      }
    resp->setStatus(http::HttpStatus::OK);
    resp->setBody(content);
    return 0;
  }

  int32_t MateriaServlet::handle(http::HttpRequest::ptr req,
				 http::HttpResponse::ptr resp,
				 http::HttpSession::ptr session)
  {
    std::string path = m_path;
    if(req->getPath()[0] == '/' && path[path.size()-1] == '/')
      path +=  req->getPath().substr(1);
    else
      path += req->getPath();
    
    std::string content;
    std::ifstream fin(path,std::ios::binary | std::ios::ate);
    if(!fin)
      {
	
	resp->setStatus(http::HttpStatus::NOT_FOUND);
	content = "<html><head><title>404 Not Found"
	  "</title></head><body><center><h1>404 Not Found</h1></center>"
	  "<hr><center>Materia Not Found</center></body></html>";
	resp->setBody(content);
	return 0;
      }
    auto remain = fin.tellg();
    auto off = 0;
    // std::cout<<"file size : "<<remain<<std::endl
    // 	     <<"path : "  << m_path<<std::endl
    // 	     <<"uri_path : "<<req->getPath()<<std::endl;
    content.resize(remain);
    while(!fin.eof() && remain != 0)
      {
	int nread = 0;
	fin.seekg(0);
	try
	  {
	    if(fin.read(&content[off],remain))
	      {
		nread = fin.gcount();
		remain -= nread;
		off+=nread;
	      }
	    else
	      {
		LOG_ROOT_ERROR() << "failed to read file <"<<m_path<<">";
		break;
	      }
	  }
	catch(std::exception& e)
	  {
	    LOG_ROOT_ERROR() << "failed to read file <" <<m_path<<">"
			     <<" error : "<<e.what();
	    break;
	  }
      }
    resp->setStatus(http::HttpStatus::OK);
    resp->setBody(content);
    return 0;
  }


  int32_t LogInServlet::handle(http::HttpRequest::ptr req,
			       http::HttpResponse::ptr resp,
			       http::HttpSession::ptr session)
  {
    std::unordered_map<std::string,std::string> form;
    req->getForm(form);
    std::string username,passwd;
    //表单错误
    if(hasValue("username",form,username)  == false
       || hasValue("passwd",form,passwd) == false)
      {
	std::cout<<username << " pwd : " <<passwd<<std::endl;
	resp->setStatus(http::HttpStatus::NOT_FOUND);
	std::string content = "<html><head><title>404 Not Found"
	  "</title></head><body><center><h1>404 Not Found</h1></center>"
	  "<hr><center>Invalid Form</center></body></html>";
	resp->setBody(content);
	return 0;
      }
    db::MySQLConn::ptr conn = db::MySQLConn::Create("root");
    db::MySQLStmtCommand::ptr cmd(new db::MySQLStmtCommand(m_stmt));

    db::IDBRes::ptr res = cmd->command(username,passwd);
    if(res->getErrno() != 0)
      {
	LOG_ROOT_ERROR() << res->getError();
      }
    if(res->getRowCount() != 0)
      {
	LOG_ROOT_INFO() << "login success";
      }
    else
      {
	LOG_ROOT_INFO() << "login failed";
      }
    resp->setStatus(http::HttpStatus::TEMPORARY_REDIRECT);
    resp->setHeader("Location","/");
    
    return 0;
  }

  int32_t RegisterServlet::handle(http::HttpRequest::ptr req,
				  http::HttpResponse::ptr resp,
				  http::HttpSession::ptr session)
  {
    std::unordered_map<std::string,std::string> form;
    req->getForm(form);
    std::string username,passwd,email;
    //表单错误
    if(hasValue("username",form,username)  == false
       || hasValue("passwd",form,passwd) == false
       || hasValue("email",form,email) == false)
      {
	resp->setStatus(http::HttpStatus::NOT_FOUND);
	std::string content = "<html><head><title>404 Not Found"
	  "</title></head><body><center><h1>404 Not Found</h1></center>"
	  "<hr><center>Register Invalid Form</center></body></html>";
	resp->setBody(content);
	return 0;
      }
    db::MySQLConn::ptr conn = db::MySQLConn::Create("root");
    db::MySQLStmtCommand::ptr cmd(new db::MySQLStmtCommand(m_stmt));

    LOG_ROOT_INFO() << std::endl
		    <<"username : "<<username<< std::endl
		    <<"passwd : "<<passwd<< std::endl
		    <<"email : "<< email << std::endl;
    int rt = cmd->execute(username,email,passwd);
    if(rt != 0)
      {
	LOG_ROOT_ERROR() << cmd->getError();
      }
    if(cmd->getAffectedRow() == 0)
      {
	LOG_ROOT_INFO() << "register failed";
      }
    else
      {
	LOG_ROOT_INFO() << "register success";
      }
    resp->setStatus(http::HttpStatus::TEMPORARY_REDIRECT);
    resp->setHeader("Location","/");
    return 0;
  }
}
}
