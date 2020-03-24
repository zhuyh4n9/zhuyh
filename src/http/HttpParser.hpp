#pragma once

#include "http11_parser.hpp"
#include "httpclient_parser.hpp"
#include "Http.hpp"

namespace zhuyh
{
namespace http
{
  enum HttpError
    {
     HTTP_OK = 0,
     METHOD_NOT_VALID,
     STATUS_NOT_VALID,
     VERSION_NOT_VALID,
     FIELD_NOT_VALID
    };
  
  class HttpRequestParser
  {
  public:
    typedef std::shared_ptr<HttpRequestParser> ptr;
    HttpRequestParser();
    
    size_t execute(char *data, size_t len);
    int isFinished();
    int hasError();
    void setError(int v)
    {
      m_error = v;
    }
    uint64_t getContentLength() const;
    HttpRequest::ptr getData() const
    {
      return m_data;
    }
    const http_parser& getParser() const
    {
      return m_parser;
    }
    static uint64_t getMaxHeaderSize();
    static uint64_t getMaxBodySize();
  private:
    http_parser m_parser;
    HttpRequest::ptr m_data;
    int m_error;
  };

  class HttpResponseParser
  {
  public:
    typedef std::shared_ptr<HttpResponseParser> ptr;
    HttpResponseParser();
    
    size_t execute(char *data, size_t len,bool chunk);
    int isFinished();
    int hasError();
    void setError(int v)
    {
      m_error = v;
    }
    static uint64_t getMaxHeaderSize();
    static uint64_t getMaxBodySize();
    uint64_t getContentLength() const;
    HttpResponse::ptr getData() const
    {
      return m_data;
    }
    const httpclient_parser& getParser() const
    {
      return m_parser;
    }
  private:
    httpclient_parser m_parser;
    HttpResponse::ptr m_data;
    int m_error;
  };
}
}
