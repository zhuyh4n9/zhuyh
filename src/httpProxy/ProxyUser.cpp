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
  
  void ProxyUser::handleServer()
  {
    //LOG_ROOT_ERROR() << "handleServer Start";
    std::shared_ptr<char> data(new char[4096],[](char* ptr){ delete [] ptr;});
    auto buff = data.get();    
    int rt = 0;
    while((rt = m_server->read(buff,4096))>0)
      {
    	//LOG_ROOT_ERROR() << "Recv From Server rt : " <<rt;
    	rt = m_client->writeFixSize(buff,rt);
    	if(rt < 0)
    	  {
    	    LOG_ROOT_ERROR() << "failed to write to client, rt = " <<rt
    			     <<" error"<<strerror(errno)<<" errno : "<<errno;
    	    break;
    	  }
	co_yield;
      }
    m_client->close();
    m_server->close();
    //LOG_ROOT_ERROR() << "Handle Server EXITING";

  }
  
  void ProxyUser::start()
  {
    co std::bind(&ProxyUser::handleServer,shared_from_this());
  }

}
}
