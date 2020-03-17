#pragma once

#include <memory>
#include "../stream/SocketStream.hpp"
#include "../http/Http.hpp"

namespace zhuyh
{
namespace http
{
  class HttpSession : public SocketStream
  {
  public:
    typedef  std::shared_ptr<HttpSession> ptr;
    HttpSession(Socket::ptr sock,bool owner = true);
    HttpRequest::ptr recvRequest();
    int sendResponse(HttpResponse::ptr resp);
  };
}
}
