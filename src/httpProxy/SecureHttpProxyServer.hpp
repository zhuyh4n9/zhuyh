#pragma once

#include"HttpProxyServer.hpp"

namespace zhuyh
{
namespace proxy
{
  class SecureHttpProxyServer : public HttpProxyServer
  {
  public:
    SecureHttpProxyServer(Scheduler* schd = nullptr,
			  Scheduler* accept_schd = nullptr,
			  const std::string& name = "SecureHttpProxyServer/1.0.0")
      :HttpProxyServer(schd,accept_schd,name){}
    void handleClient(Socket::ptr client) override;
  };
}
}
