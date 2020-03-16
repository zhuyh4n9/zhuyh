#pragma once

#include <memory>
#include "Socket.hpp"
#include "Address.hpp"
#include "../scheduler/Scheduler.hpp"

namespace zhuyh
{
  class TcpServer : public std::enable_shared_from_this<TcpServer>
  {
  public:
    typedef std::shared_ptr<TcpServer> ptr;
    TcpServer(Scheduler* schd = nullptr,
	      Scheduler* accept_schd = nullptr,
	      const std::string& name = "tcpserver/1.0.0");
    virtual ~TcpServer();
    virtual bool bind(IAddress::ptr address);
    virtual bool bind(const std::vector<IAddress::ptr>& address,
		      std::vector<IAddress::ptr>& failed);
    virtual bool start();
    virtual bool stop();

    uint64_t getReadTimeout() const
    {
      return m_recvTimeout;
    }
    void setReadTimout(uint64_t v)
    {
      m_recvTimeout = v;
    }

    bool isStop() const
    {
      return m_stop;
    }
  private:
    TcpServer(const TcpServer& ) = delete;
    TcpServer& operator<<(const TcpServer&) = delete;
  private:
    Socket::ptr bind(uint32_t family);
  protected:
    virtual void handleClient(Socket::ptr client);
    virtual void startAccept(Socket::ptr sock);
  private:
    std::vector<Socket::ptr> m_socks;
    Scheduler* m_schd;
    Scheduler* m_acceptSchd;
    uint64_t m_recvTimeout;
    std::string m_name;
    bool m_stop;
  };
}
