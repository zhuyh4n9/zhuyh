#include"ProxyUser.hpp"
#include"../macro.hpp"
#include"../co.hpp"
namespace zhuyh
{
namespace proxy
{

  ProxyUser::ptr ProxyUser::Create(http::HttpSession::ptr client,
				   SocketStream::ptr server)
  {
    if(client == nullptr || server == nullptr) return nullptr;
    ProxyUser::ptr user(new ProxyUser(client,server,client->getId()));
    return user;
  }
  void ProxyUser::handleClient()
  {
    std::shared_ptr<char> data(new char[40960],[](char* ptr){ delete [] ptr;});
    auto buff = data.get();    
    int rt = 0;
    bool start = false;
    while((rt = m_client->read(buff,40960))>0)
      {
	rt = m_server->writeFixSize(buff,rt);
	if(rt < 0)
	  {
	    LOG_ROOT_ERROR() << "failed to write to target server, rt = " <<rt
			     <<" error"<<strerror(errno)<<" errno : "<<errno;
	    break;
	  }
	if(!start)
	  {
	    co std::bind(&ProxyUser::handleServer,shared_from_this());
	    start = true;
	  }
	co_yield;
      }
    //m_server->close();
  }
  
  void ProxyUser::handleServer()
  {
    std::shared_ptr<char> data(new char[40960],[](char* ptr){ delete [] ptr;});
    auto buff = data.get();    
    int rt = 0;
    while((rt = m_server->read(buff,40960))>0)
      {
	rt = m_client->writeFixSize(buff,rt);
	if(rt < 0)
	  {
	    LOG_ROOT_ERROR() << "failed to write to client, rt = " <<rt
			     <<" error"<<strerror(errno)<<" errno : "<<errno;
	    break;
	  }
	co_yield;
      }
    //m_client->close();
  }

  void ProxyUser::start()
  {
    static bool started = false;
    if(!started)
      co std::bind(&ProxyUser::handleClient,shared_from_this());
  }

}
}
