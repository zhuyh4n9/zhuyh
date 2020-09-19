#pragma once

#include <memory>
#include "HttpParser.hpp"
#include "Http.hpp"
#include "socket/Socket.hpp"
#include "stream/SocketStream.hpp"

namespace zhuyh
{
namespace http
{

  class HttpConnection : public SocketStream
  {
  public:
    typedef std::shared_ptr<HttpConnection> ptr;
    HttpConnection(Socket::ptr sock,bool owner = true);
    int sendRequest(HttpRequest::ptr req);
    HttpResponse::ptr recvResponse();
    bool recvResponse(HttpResponse::ptr& resp)
    {
      auto tmp = recvResponse();
      if(tmp == nullptr) return false;
      resp = tmp;
      return true;
    }
  };

}
}
