#include "Http.hpp"
#include <vector>
#include <sstream>
#include "../logs.hpp"

namespace zhuyh
{
namespace http
{

  bool CaseInsensitiveLesser::operator()(const std::string& lhs,
					 const std::string& rhs) const
  {
    return strcasecmp(lhs.c_str(),rhs.c_str()) < 0;
  }

  
  static  Logger::ptr s_logger = GET_LOGGER("system");


  //只读且文件作用域
  static const std::vector<std::string>& s_method_to_string()
  {
    static const std::vector<std::string> _method_to_string =
    {
#define XX(num,name,string)        #string,
     HTTP_METHOD_MAP(XX)
#undef XX
     "INVALID METHOD"
    };
    return _method_to_string;
  }
  
  static const std::map<std::string,HttpMethod,CaseInsensitiveLesser>&
    s_string_to_method()
  {
    static const std::map<std::string,HttpMethod,CaseInsensitiveLesser>
      _string_to_method = 
      {
#define XX(num,name,string) { #string,(HttpMethod)num},
       HTTP_METHOD_MAP(XX)
#undef XX
      };
    return _string_to_method;
  }
  
  static const std::map<int,std::string> s_status_to_string()
  {
    static std::map<int,std::string>
      _status_to_string
      {
#define XX(num,name,str) {(int)HttpStatus::name, str},
       HTTP_STATUS_MAP(XX)
#undef XX
      };
    return _status_to_string;
  }
  static const std::map<std::string,HttpStatus>& s_string_to_status()
  {
    static std::map<std::string,HttpStatus> _string_to_status =
      {
#define XX(num,name,string) {string,(HttpStatus)num},
       HTTP_STATUS_MAP(XX)
#undef XX
      };
    return _string_to_status;
  }
  //end of maps;


  std::string httpMethodToString(HttpMethod method)
  {
    size_t idx = (size_t)method;
    if(idx > s_method_to_string().size())
      return "INVALID METHOD";
    return s_method_to_string()[idx];
  }
  HttpMethod  stringToHttpMethod(const std::string&  method)
  {
    auto it = s_string_to_method().find(method);
    if(it == s_string_to_method().end())
      {
	return HttpMethod::INVALID_METHOD;
      }
    return it->second;
  }
  
  std::string httpStatusToString(HttpStatus status,int pos = 1)
  {
    //LOG_ROOT_INFO() << "idx : "<<(int)status << " pos : "<<pos;
    auto& mp = s_status_to_string();
    //LOG_ROOT_INFO()<<"HERE\n";
    auto it = mp.find((int)status);
    return it == mp.end() ? "INVALID STATUS":it->second;
    
  }
  HttpStatus  stringToHttpStatus(const std::string& status)
  {
    auto& mp = s_string_to_status();
    auto it = mp.find(status);
    return it == mp.end() ? HttpStatus::INVALID_STATUS : it->second;
  }
  HttpRequest::HttpRequest(uint8_t version,bool close)
    :m_method(HttpMethod::GET),
     m_version(version),
     m_close(close),
     m_webSocket(false),
     m_status(HttpStatus::OK),
     m_path("/")
  {
  }
  
  std::string HttpRequest::getHeader(const std::string& key,const std::string& dft) const
  {
    auto it = m_headers.find(key);
    return it == m_headers.end() ? dft : it->second;
  }
  std::string HttpRequest::getParam(const std::string& key,const std::string& dft) const
  {
    auto it = m_params.find(key);
    return it == m_params.end() ? dft : it->second;
  }
  std::string HttpRequest::getCookie(const std::string& key,const std::string& dft) const
  {
    auto it = m_cookies.find(key);
    return it == m_cookies.end() ? dft : it->second;
  }

  bool HttpRequest::getForm(std::unordered_map<std::string,std::string>& mp)
  {
#define KEY 0
#define VALUE 1
    if(m_body.empty()) return false;
    std::string kv[2];
    //type = 0 : key , type = 1 : value
    int type = KEY;
    for(size_t i=0;i<m_body.size();i++)
      {
	if(m_body[i] == '&')
	  {
	    mp[kv[KEY]] = kv[VALUE];
	    kv[KEY].clear();
	    kv[VALUE].clear();
	    type = KEY;
	  }
	else if(m_body[i] == '=')
	  {
	    type = VALUE;
	  }
	else
	  {
	    kv[type]+=m_body[i];
	  }
      }
    if(type != VALUE) return false;
    mp[kv[KEY]] = kv[VALUE];
    return true;
#undef KEY
#undef VALUE
  }
  void HttpRequest::setHeader(const std::string& key,const std::string& val)
  {
    m_headers[key] = val;
  }
  void HttpRequest::setParam(const std::string& key,const std::string& val)
  {
    m_params[key] = val;
  }
  void HttpRequest::setCookie(const std::string& key,const std::string& val)
  {
    m_cookies[key] = val;
  }
  
  void HttpRequest::delHeader(const std::string& key)
  {
    m_headers.erase(key);
  }
  void HttpRequest::delParam(const std::string& key)
  {
    m_params.erase(key);
  }
  void HttpRequest::delCookie(const std::string& key)
  {
    m_cookies.erase(key);
  }

  bool HttpRequest::hasHeader(const std::string& key,std::string* val)
  {
    auto it = m_headers.find(key);
    if(it == m_headers.end())
      {
	val = nullptr;
	return false;
      }
    if(val)
      *val = it->second;
    return false;
  }
  bool HttpRequest::hasParam(const std::string& key,std::string* val)
  {
    auto it = m_params.find(key);
    if(it == m_params.end())
      {
	val = nullptr;
	return false;
      }
    if(val)
      *val = it->second;
    return false;
  }
  bool HttpRequest::hasCookie(const std::string& key,std::string* val)
  {
    auto it = m_cookies.find(key);
    if(it == m_cookies.end())
      {
	val = nullptr;
	return false;
      }
    if(val)
      *val = it->second;
    return false;
  }
  
  std::ostream& HttpRequest::dump(std::ostream& os) const
  {
    os << httpMethodToString(m_method)<<" ";
    if(getMethod() == HttpMethod::CONNECT)
      {
	os <<  m_scheme +":"<<m_path;
      }
    else
      {
	os<< (m_scheme.empty() ? "" : m_scheme+"://")<<m_path
	  << (m_query.empty() ? "":"?")<<m_query
	  << (m_fragment.empty() ? "":"#")<<m_fragment;
      }
    os << " HTTP/"<<m_version/10<<"."<<m_version%10<<"\r\n";
    for(auto& item : m_headers)
      {
	if(!m_webSocket && strcasecmp(item.first.c_str(),"connection") == 0)
	  continue;
	os << item.first << ": "<<item.second<<"\r\n";
      }
    if(!m_webSocket)
      os << "Connection: "<<(m_close ? "close" : "keep-alive")<<"\r\n";
    if(!m_body.empty())
      {
	os << "Content-length :"<<m_body.size()
	   <<"\r\n\r\n"<< m_body;
      }
    else
      os << "\r\n";
    return os;
  }
  std::string HttpRequest::toString() const
  {
    std::stringstream ss;
    dump(ss);
    return ss.str();
  }
  
  void HttpRequest::init()
  {
    std::string conn = getHeader("connection");
    if(!conn.empty())
      {
	if(strcasecmp(conn.c_str(),"keep-alive") == 0)
	  m_close = false;
	else
	  m_close = true;
      }
  }
  void HttpRequest::initParam()
  {
    initQueryParam();
    initBodyParam();
    initCookies();
  }
  
  void HttpRequest::initQueryParam()
  {
    //#define PARSE_PARAM(str,
  }
  void HttpRequest::initBodyParam()
  {
  }
  void HttpRequest::initCookies()
  {
  }

  HttpResponse::HttpResponse(uint8_t version ,bool close )
    :m_status(HttpStatus::OK),
     m_version(version),
     m_close(close),
     m_webSocket(false)
  {
  }
  std::string HttpResponse::getHeader(const std::string& key,const std::string& dft) const
  {
    auto it = m_headers.find(key);
    return it == m_headers.end() ? dft : it->second;
  }
  void HttpResponse::setHeader(const std::string& key,const std::string& val)
  {
    m_headers[key] = val;
  }
  void HttpResponse::delHeader(const std::string& key)
  {
    m_headers.erase(key);
  }
  bool HttpResponse::hasHeader(const std::string& key,std::string* val)
  {
    auto it = m_headers.find(key);
    if(it == m_headers.end())
      {
	val = nullptr;
	return false;
      }
    if(val)
      *val = it->second;
    return false;
  }
  void HttpResponse::setCookie(const std::string& key, const std::string& val,
			       time_t expired, const std::string& path,
			       const std::string& domain, bool secure){
    std::stringstream ss;
    ss << key << "=" << val;
    if(expired > 0) {
        ss << ";expires=" <<time2Str(expired, "%a, %d %b %Y %H:%M:%S") << " GMT";
    }
    if(!domain.empty()) {
        ss << ";domain=" << domain;
    }
    if(!path.empty()) {
        ss << ";path=" << path;
    }
    if(secure) {
        ss << ";secure";
    }
    m_cookies.push_back(ss.str());
  }
  std::ostream& HttpResponse::dump(std::ostream& os) const
  {
    os << "HTTP/"<<m_version/10<<"."<<m_version%10
       << " "<<(uint32_t)m_status<<" "
       <<(m_reason.empty() ? httpStatusToString(m_status,2) : m_reason)
       <<"\r\n";

    for(auto& item: m_headers)
      {
	if(!m_webSocket && strcasecmp(item.first.c_str(),"connection") == 0)
	  continue;
	os << item.first<<": "<<item.second<<"\r\n";
      }
    for(auto& i : m_cookies) {
      os << "Set-Cookie: " << i << "\r\n";
    }
    if(!m_webSocket)
      os << "connection: "<<(m_close ? "close" : "keep-alive")<<"\r\n";
    if(!m_body.empty())
      {
	os << "content-lentgh: "<<m_body.size()<<"\r\n\r\n"
	   <<m_body;
      }
    else
      os << "\r\n";
    return os;
  }
  
  std::string HttpResponse::toString() const
  {
    std::stringstream ss;
    dump(ss);
    return ss.str();
  }
  
  std::ostream& operator<<(std::ostream& os,
			   const HttpRequest& req)
  {
    req.dump(os);
    return os;
  }
  
  std::ostream& operator<<(std::ostream& os,
			   const HttpResponse& rep)
  {
    rep.dump(os);
    return os;
  }
}
}
