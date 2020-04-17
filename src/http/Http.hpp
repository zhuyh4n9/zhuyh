#pragma once

#include <memory>
#include <boost/lexical_cast.hpp>
#include <map>
#include <boost/exception/all.hpp>
#include "../logs.hpp"
#include <vector>

namespace zhuyh
{
namespace http
{
#define HTTP_METHOD_MAP(XX)		    \
    XX(0,DELETE, DELETE)		    \
    XX(1,GET,  GET)			    \
    XX(2,HEAD, HEAD)			    \
    XX(3,POST, POST)			    \
    XX(4,PUT,  PUT)			    \
    XX(5,CONNECT, CONNECT)		    \
    XX(6,OPTION, OPTION)		    \
    XX(7,TRACE,TRACE)			    \
    XX(8,COPY,COPY)			    \
    XX(9,LOCK,LOCK)			    \
    XX(10,MKCOL,MKCOL)			    \
    XX(11,MOVE,MOVE)			    \
    XX(12,PROPFIND, PROPFIND)		    \
    XX(13,PROPPATCH, PROPPATCH)		    \
    XX(14,SEARCH,SEARCH)		    \
    XX(15,UNLOCK,UNLOCK)		    \
    XX(16,BIND,BIND)			    \
    XX(17,REBIND,REBIND)		    \
    XX(18,UNBIND,UNBIND)		    \
    XX(19,ACL,ACL)			    \
    XX(20,REPORT,REPORT)		    \
    XX(21,MKACTIVITY,MKACTIVITY)	    \
    XX(22,CHECKOUT,CHECKOUT)		    \
    XX(23,MERGE,MERGE)			    \
    XX(24,MSEARCH,M-SEARCH)		    \
    XX(25,NOTIFY,NOTIFY)		    \
    XX(26,SUBSCRIBE,SUBSRIBE)		    \
    XX(27,UNSUBSCRIBE,UNSUBSCRIBE)	    \
    XX(28,PATCH,PATCH)			    \
    XX(29,PURGE,PURGE)			    \
    XX(30,MKCALENDAR,MKCALENDAR)	    \
    XX(31,LINK,LINK)			    \
    XX(32,UNLINK,UNLINK)		    \
    XX(33,SOURCE,SOURCE)

#define HTTP_STATUS_MAP(XX)						\
  XX(100,  CONTINUE,                        "Continue")			\
  XX(101,  SWITCHING_PROTOCOLS,             "Switching Protocols")	\
  XX(102,  PROCESSING,                      "Processing")		\
  XX(200,  OK,                              "OK")			\
  XX(201,  CREATED,                         "Created")			\
  XX(202,  ACCEPTED,                        "Accepted")			\
  XX(203,  NON_AUTHORITATIVE_INFORMATION,   "Non Authoritative Infomation") \
  XX(204,  NO_CONTENT,                      "No Content")		\
  XX(205,  RESET_CONTENT,                   "Reset Content")		\
  XX(206,  PARTIAL_CONTENT,                 "Partial Content")		\
  XX(207,  MULTI_STATUS,                    "Multi Status")		\
  XX(208,  ALREADY_REPORTED,                "Already reported")		\
  XX(226,  IM_USED,                         "IM-Used")			\
  XX(300,  MULTI_CHOICES,                   "Multi Choices")		\
  XX(301,  MOVED_PERMANENTLY,               "Moved Permanently")	\
  XX(302,  FOUND,                           "Found")			\
  XX(303,  SEE_OTHER,                       "See Other")		\
  XX(304,  NOT_MODIFIED,                    "Not Modified")		\
  XX(305,  USE_PROXY,                       "Use Proxy")		\
  XX(307,  TEMPORARY_REDIRECT,              "Temporary Redirect")	\
  XX(308,  PERMANENT_REDIRECT,              "Permanent Redirect")	\
  XX(400,  BAD_REQUEST,                     "Bad Request")		\
  XX(401,  UNAUTHORIZED,                    "Unauthorized")		\
  XX(402,  PAYMENT_REQUIRED,                "Payment Requried")		\
  XX(403,  FORBIDDEN,                       "Forbidden")		\
  XX(404,  NOT_FOUND,                       "Not Found")		\
  XX(405,  METHOD_NOT_ALLOWED,              "Method Not Allowed")	\
  XX(406,  NOT_ACCEPTABLE,                  "Not Acceptable")		\
  XX(407,  PROXY_AUTHENTICATION_REQURIED,   "Proxy Authentication Requried") \
  XX(408,  REQUEST_TIMEOUT,                 "Request Timeout")		\
  XX(409,  CONFLICT,                        "Conflic")			\
  XX(410,  GONE,                            "GONE")			\
  XX(411,  LENGTH_REQUIRED,                 "Length Required")		\
  XX(412,  PRECONDITION_FAILED,             "Precondition Failed")	\
  XX(413,  PAYLOAD_TOO_LARGE,               "Payload too large")	\
  XX(414,  URI_TOO_LONG,                    "Uri Too Long")		\
  XX(415,  UNSUPPORTED_MEDIA_TYPE,          "Unsupported Media Type")	\
  XX(416,  RANGE_NOT_SATISFIABLE,           "Range Not Satisfiable")	\
  XX(417,  EXPECTATION_FAILED,              "Expectation Failed")	\
  XX(421,  MISDIRECTED_REQUEST,            " Misdirected Request")	\
  XX(422,  UNPROCESSABLE_ENTITY,            "Unprocessable Entity")	\
  XX(423,  LOCKED,                          "Locked")			\
  XX(424,  FAILED_DEPENDENCY,               "Failed Dependency")	\
  XX(426,  UPGRADE_REQUIRED,                "Upgrade Required")		\
  XX(428,  PRECONDITION_REQUIRED,           "Precondition Required")	\
  XX(429,  TOO_MANY_REQUESTS,               "Too Many Requests")	\
  XX(431,  REQUEST_HEADER_FIELDS_TOO_LARGE, "Request Header Fields Too Large") \
  XX(451,  UNAVAILABLE_FOR_LEGAL_REASONS,   "Unavailable For Legal Reasons") \
  XX(500,  INTERNAL_SERVER_ERROR,           "Internal Server Error")	\
  XX(501,  NOT_IMPLEMENTED,                 "Not Implemented")		\
  XX(502,  BAD_GATEWAY,                     "Bad Gateway")		\
  XX(503,  SERVICE_UNAVAILABLE,             "Service Unavailable")	\
  XX(504,  GATEWAY_TIMEOUT,                 "Gateway Timeout")		\
  XX(505,  HTTP_VERSION_NOT_SUPPORTED,      "HTTP Version Not Supported") \
  XX(506,  VARIANT_ALSO_NEGOTIATES,         "Variant Also Negotiates")	\
  XX(507,  INSUFFICIENT_STORAGE,            "Insufficient Storage")	\
  XX(508,  LOOP_DETECTED,                   "Loop Detected")		\
  XX(510,  NOT_EXTENDED,                    "Not Extended")		\
  XX(511,  NETWORK_AUTHENTICATION_REQUIRED, "Network Authentication Required")
  
  enum class HttpMethod
    {
#define XX(num,name,desc) name = num,
     HTTP_METHOD_MAP(XX)
#undef XX
     INVALID_METHOD
    };
  
  enum class HttpStatus
    {
#define XX(code,name,desc) name = code,
     HTTP_STATUS_MAP(XX)
#undef XX
     INVALID_STATUS
    };
  
  struct CaseInsensitiveLesser
  {
    bool operator()(const std::string& lhs,const std::string& rhs) const;
  };
  using MapType = std::map<std::string,std::string,CaseInsensitiveLesser> ;
  
  template<class T>
  bool checkGetAs(const MapType& mp,const std::string& key,
		  T& val,const T& dft = T())
  {
    auto it = mp.find(key);
    if(it == mp.end())
      {
	val = dft;
	return false;
      }
    try
      {
	val = boost::lexical_cast<T>(it->second);
	return true;
      }
    catch(boost::exception& e)
      {
	val = dft;
	return false;
      }
    catch(...)
      {
	auto logger = GET_LOGGER("system");
	LOG_ERROR(logger) << "lexical_cast failed , unknown error";
	val = dft;
	return false;
      }
  }
  
  template<class T>
  T getAs(const MapType& mp,const std::string& key,const T& dft = T())
  {
    auto it = mp.find(key);
    if(it == mp.end())
      {
	return dft;
      }
    try
      {
	T val = boost::lexical_cast<T>(it->second);
	return val;
      }
    catch(boost::exception& e)
      {
	return dft;
      }
    catch(...)
      {
	auto logger = GET_LOGGER("system");
	LOG_ERROR(logger) << "lexical_cast failed , unknown error";
	return dft;
      }
  }
  std::string httpMethodToString(HttpMethod method);
  HttpMethod  stringToHttpMethod(const std::string&  method);
  
  std::string httpStatusToString(HttpStatus status);
  HttpStatus  stringToHttpStatus(const std::string& status);

  
  class HttpRequest
  {
  public:
    typedef std::shared_ptr<HttpRequest> ptr;
    HttpRequest(uint8_t version = 11,bool close = true);
    
    HttpMethod getMehod() const
    {
      return m_method;
    }
    HttpStatus getStatus() const
    {
      return m_status;
    }
    const std::string& getPath() const
    {
      return m_path;
    }
    const std::string& getQuery() const
    {
      return m_query;
    }

    const std::string& getUri() const
    {
      return m_uri;
    }
    const std::string& getScheme() const
    {
      return m_scheme;
    }
    const std::string& getFragment() const
    {
      return m_fragment;
    }

    const std::string& getBody() const
    {
      return m_body;
    }

    const MapType& getHeaders() const
    {
      return m_headers;
    }

    const MapType& getParams() const
    {
      return m_params;
    }

    const MapType& getCookies() const
    {
      return m_cookies;
    }

    uint8_t getVersion() const
    {
      return m_version;
    }
    bool isClose() const
    {
      return m_close;
    }
    void setStatus(HttpStatus status)
    {
      m_status = status;
    }
    void setVersion(uint8_t version)
    {
      m_version = version;
    }

    void setMethod(HttpMethod method)
    {
      m_method = method;
    }

    void setPath(const std::string& path)
    {
      m_path = path;
    }
    void setQuery(const std::string& query)
    {
      m_query = query;
    }
    void setFragment(const std::string& fragment)
    {
      m_fragment = fragment;
    }
    void setBody(const std::string& body)
    {
      m_body = body;
    }
    void setUri(const std::string& uri)
    {
      m_uri = uri;
    }
    void setScheme(const std::string& scheme)
    {
      m_scheme = scheme;
    }
    void setClose(bool _close)
    {
      m_close = _close;
    }
    void setHeaders(const MapType& headers)
    {
      m_headers = headers;
    }

    void setParams(const MapType& params)
    {
      m_params = params;
    }

    void setCookies(const MapType& cookies)
    {
      m_cookies = cookies;
    }
    
    std::string getHeader(const std::string& key,const std::string& dft = "") const;
    std::string getParam(const std::string& key,const std::string& dft = "") const;
    std::string getCookie(const std::string& key,const std::string& dft = "") const;

    void setHeader(const std::string& key,const std::string& val);
    void setParam(const std::string& key,const std::string& val);
    void setCookie(const std::string& key,const std::string& val);
    
    void delHeader(const std::string& key);
    void delParam(const std::string& key);
    void delCookie(const std::string& key);

    bool hasHeader(const std::string& key,std::string* val = nullptr);
    bool hasParam(const std::string& key,std::string* val = nullptr);
    bool hasCookie(const std::string& key,std::string* val = nullptr);
  public:
    template<class T>
    bool checkGetHeaderAs(const std::string& key,T& val,const T& dft = T())
    {
      return checkGetAs(m_headers,key,val,dft);
    }
    template<class T>
    T getHeaderAs(const std::string& key,const T& dft = T())
    {
      return getAs(m_headers,key,dft);
    }
    
    template<class T>
    bool checkGetParamAs(const std::string& key,T& val,const T& dft = T())
    {
      return checkGetAs(m_params,key,val,dft);
    }
    template<class T>
    T getParamAs(const std::string& key,const T& dft = T())
    {
      return getAs(m_params,key,dft);
    }

    template<class T>
    bool checkGetCookiesAs(const std::string& key,T& val,const T& dft = T())
    {
      return checkGetAs(m_cookies,key,val,dft);
    }
    template<class T>
    T getCookiesAs(const std::string& key,const T& dft = T())
    {
      return getAs(m_cookies,key,dft);
    }
    std::ostream& dump(std::ostream& os) const;
    std::string toString() const;

    void init();
    void initParam();
    void initQueryParam();
    void initBodyParam();
    void initCookies();
  private:
    HttpMethod m_method = HttpMethod::INVALID_METHOD;
    uint8_t m_version = 11;
    bool m_close;
    HttpStatus  m_status = HttpStatus::INVALID_STATUS;
    
    std::string m_path;
    std::string m_query;
    std::string m_fragment;
    std::string m_body;
    std::string m_uri;
    std::string m_scheme;
    
    MapType m_headers;
    MapType m_params;
    MapType m_cookies;
  };

  
  class HttpResponse
  {
  public:
    typedef std::shared_ptr<HttpResponse> ptr;
    HttpResponse(uint8_t version = 11,bool close  = false);
    HttpStatus getStatus() const
    {
      return m_status;
    }
    uint8_t getVersion() const
    {
      return m_version;
    }
    const std::string& getBody() const
    {
      return m_body;
    }
    const std::string& getReason() const
    {
      return m_reason;
    }
    const MapType& getHeaders() const
    {
      return m_headers;
    }
    const std::vector<std::string>& getCookie() const
    {
      return m_cookies;
    }
    
    void setStatus(HttpStatus status)
    {
      m_status = status;
    }

    void setVersion(uint8_t version)
    {
      m_version = version;
    }

    void setBody(const std::string& body)
    {
      m_body = body;
    }

    void setReason(const std::string& reason)
    {
      m_reason = reason;
    }
    void setHeaders(const MapType& headers)
    {
      m_headers = headers;
    }

    void setCookies(const std::vector<std::string>& cookies)
    {
      m_cookies = cookies;
    }

    bool isClose() const
    {
      return m_close;
    }
    void setClose(bool _close)
    {
      m_close = _close;
    }

    std::string getHeader(const std::string& key,const std::string& dft = "") const;
    void setHeader(const std::string& key,const std::string& val);
    void delHeader(const std::string& key);
    bool hasHeader(const std::string& key,std::string* val = nullptr);

    std::ostream& dump(std::ostream& os) const;
    std::string toString() const;
  public:
    template<class T>
    bool checkGetHeaderAs(const std::string& key,T& val,const T& dft = T())
    {
      return checkGetAs(m_headers,key,val,dft);
    }
    template<class T>
    T getHeaderAs(const std::string& key,const T& dft = T())
    {
      return getAs(m_headers,key,dft);
    }
    
  private:
    HttpStatus m_status;
    uint8_t m_version;
    bool m_close;
    
    std::string m_body;
    std::string m_reason;

    MapType m_headers;
    std::vector<std::string> m_cookies;
    
  };
  
  std::ostream& operator<<(std::ostream& os,
			   const HttpRequest& req);
  std::ostream& operator<<(std::ostream& os,
			   const HttpResponse& rep);
}
}
