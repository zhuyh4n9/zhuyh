#include"HttpParser.hpp"
#include"../logs.hpp"

namespace zhuyh
{
namespace http
{
  
  static Logger::ptr s_logger = GET_LOGGER("system");
  //最大请求包大小
  static ConfigVar<uint64_t>::ptr s_http_request_header_size =
    Config::lookUp<uint64_t>("http.request.header_size",4*1024ull,"http max request header size");

  //最大响应包大小
  static ConfigVar<uint64_t>::ptr s_http_request_body_size =
    Config::lookUp<uint64_t>("http.request.body_size",64*1024*1024ull,
			     "http max request body size");

  static ConfigVar<uint64_t>::ptr s_http_response_header_size =
    Config::lookUp<uint64_t>("http.response.header_size",4*1024ull,"http max response header size");
  
  //最大响应包大小
  static ConfigVar<uint64_t>::ptr s_http_response_body_size =
    Config::lookUp<uint64_t>("http.response.body_size",64*1024*1024ull,
			     "http max response body size");
  
  static uint64_t _http_response_header_size;
  static uint64_t _http_response_body_size;
  
  static uint64_t _http_request_header_size;
  static uint64_t _http_request_body_size;
  struct __RequestIniter
  {
    __RequestIniter()
    {
      _http_request_header_size = s_http_request_header_size->getVar();
      _http_request_body_size = s_http_request_body_size->getVar();
      
      s_http_request_header_size
	->addCb([](const uint64_t& ov,const uint64_t& nv)
		{
		  _http_request_header_size = nv;
		});
      s_http_request_body_size
	->addCb([](const uint64_t& ov,const uint64_t& nv)
		{
		  _http_request_body_size = nv;
		});			      
    }
  };

  struct __RequestIniter s_request_initer;
  //request callback
  void on_request_method(void *data, const char *at, size_t length)
  {
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    HttpMethod m = stringToHttpMethod(std::string(at,length));
    if(m == HttpMethod::INVALID_METHOD)
      {
	LOG_WARN(s_logger) << "invalid http request method : "
			   <<std::string(at,length);
	parser->setError(METHOD_NOT_VALID);
      }
    parser->getData()->setMethod(m);
  }
  void on_request_uri(void *data, const char *at, size_t length)
  {
  }
  void on_request_path(void *data, const char *at, size_t length)
  {
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    parser->getData()->setPath(std::string(at,length));
  }
  void on_request_fragment(void *data, const char *at, size_t length)
  {
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    parser->getData()->setFragment(std::string(at,length));
  }
  void on_request_query(void *data, const char *at, size_t length)
  {
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    parser->getData()->setQuery(std::string(at,length));
  }
  void on_request_header_done(void *data, const char *at, size_t length)
  {
  }
  void on_request_version(void *data, const char *at, size_t length)
  {
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    uint8_t version = 0;
    if(strncasecmp("HTTP/1.1",at,length) == 0)
      version = 11;
    else if(strncasecmp("HTTP/1.0",at,length) == 0)
      version = 10;
    else
      {
	LOG_WARN(s_logger) << "Invalid http version : "
			   <<std::string(at,length);
	parser->setError(VERSION_NOT_VALID);
	return ;
      }
    parser->getData()->setVersion(version);
  }
  

  void on_request_http_field(void *data, const char *field, size_t flen,
			     const char *value, size_t vlen)
  {
    HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
    if(flen == 0)
      {
	LOG_WARN(s_logger) << "Invalid flied length : length = 0";
	parser->setError(FIELD_NOT_VALID);
	return;
      }
    parser->getData()->setHeader(std::string(field,flen),
				 std::string(value,vlen));
  }


  struct __ResponseIniter
  {
    __ResponseIniter()
    {
      _http_response_header_size = s_http_response_header_size->getVar();
      _http_response_body_size = s_http_response_body_size->getVar();
      
      s_http_response_header_size
	->addCb([](const uint64_t& ov,const uint64_t& nv)
		{
		  _http_response_header_size = nv;
		});
      s_http_response_body_size
	->addCb([](const uint64_t& ov,const uint64_t& nv)
		{
		  _http_response_body_size = nv;
		});			      
    }
  };

  struct __ResponseIniter s_responser_initer;
  
  HttpRequestParser::HttpRequestParser()
    :m_error(0)
  {
    m_data.reset(new HttpRequest());
    http_parser_init(&m_parser);
    m_parser.request_method = on_request_method;
    m_parser.request_uri = on_request_uri;
    m_parser.fragment = on_request_fragment;
    m_parser.request_path = on_request_path;
    m_parser.query_string = on_request_query;
    m_parser.http_version = on_request_version;
    m_parser.header_done = on_request_header_done;

    m_parser.http_field = on_request_http_field;
    m_parser.data = this;
  }

  uint64_t HttpRequestParser::getContentLength() const
  {
    return m_data->getHeaderAs<uint64_t>("content-length",0);
  }
  size_t HttpRequestParser::execute(char *data, size_t len)
  {
    size_t offset = http_parser_execute(&m_parser,data,len,0);
    memmove(data,data+offset,len-offset);
    return offset;
  }
  int HttpRequestParser::isFinished()
  {
    return http_parser_finish(&m_parser);
  }
  int HttpRequestParser::hasError()
  {
    return m_error || http_parser_has_error(&m_parser);
  }


  //response callback
  void on_response_reason(void *data, const char *at, size_t length)
  {
    HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
    parser->getData()->setReason(std::string(at,length));
  }
  void on_response_status(void *data, const char *at, size_t length)
  {
    HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
    HttpStatus status = (HttpStatus)atoi(at);
    parser->getData()->setStatus(status);
  }
  void on_response_chunk(void *data, const char *at, size_t length)
  {
  }
  void on_response_version(void *data, const char *at, size_t length)
  {
    HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
    if(strncasecmp(at,"HTTP/1.1",length) == 0)
      parser->getData()->setVersion(11);
    else if(strncasecmp(at,"HTTP/1.0",length) == 0)
      parser->getData()->setVersion(10);
    else
      {
	LOG_WARN(s_logger) << "Invalid http version : "<<std::string(at,length);
	parser->setError(VERSION_NOT_VALID);
	return;
      }
  }
  void on_response_header_done(void *data, const char *at, size_t length)
  {
  }
  void on_response_last_chunk(void *data, const char *at, size_t length)
  {
  }

  void on_response_http_field(void *data, const char *field, size_t flen,
			       const char *value, size_t vlen)
  {
    HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
    if(flen == 0)
      {
	LOG_WARN(s_logger) << "Invalid flied length : length = 0";
	parser->setError(FIELD_NOT_VALID);
	return;
      }
    parser->getData()->setHeader(std::string(field,flen),
				 std::string(value,vlen));
  }
  HttpResponseParser::HttpResponseParser()
    :m_error(0)
  {
    m_data.reset(new HttpResponse());
    httpclient_parser_init(&m_parser);
    
    m_parser.reason_phrase = on_response_reason;
    m_parser.status_code =  on_response_status;
    m_parser.chunk_size = on_response_chunk;
    m_parser.http_version = on_response_version;
    m_parser.header_done = on_response_header_done;
    m_parser.last_chunk = on_response_last_chunk;
    
    m_parser.http_field = on_response_http_field;
    m_parser.data = this;
  }
    
  size_t HttpResponseParser::execute(char *data, size_t len,bool chunk)
  {
    if(chunk)
      {
	httpclient_parser_init(&m_parser);
      }
    size_t offset = httpclient_parser_execute(&m_parser,data,len,0);
    memmove(data,data+offset,len-offset);
    return offset;
  }
  int HttpResponseParser::isFinished()
  {
    return httpclient_parser_finish(&m_parser);
  }
  int HttpResponseParser::hasError()
  {
    return m_error || httpclient_parser_has_error(&m_parser);
  }

  uint64_t HttpResponseParser::getContentLength() const
  {
    return m_data->getHeaderAs<uint64_t>("Content-Length",0);
  }
}
}
