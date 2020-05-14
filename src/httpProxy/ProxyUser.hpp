#pragma once

#include"../Singleton.hpp"
#include<map>
#include"../latch/lock.hpp"
#include"../stream/SocketStream.hpp"
#include"../http/HttpSession.hpp"

namespace zhuyh
{
namespace proxy
{
  class ProxyUser : public std::enable_shared_from_this<ProxyUser>
  {
  public:
    //
    typedef std::shared_ptr<ProxyUser> ptr;
    static ProxyUser::ptr Create(http::HttpSession::ptr client,
				 SocketStream::ptr server);

    //~ProxyUser() { LOG_ROOT_INFO() << "ProxyUser Destroyed";}
    void handleServer();

    void start();
  private:
    ProxyUser(SocketStream::ptr client,
	      SocketStream::ptr server,
	      uint64_t id)
      :m_id(id),
       m_client(client),
       m_server(server) {}
  private:
    //session id
    uint64_t m_id;
    SocketStream::ptr m_client;
    SocketStream::ptr m_server;
  };

}
}
