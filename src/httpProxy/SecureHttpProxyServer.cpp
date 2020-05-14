#include"SecureHttpProxyServer.hpp"
#include"ProxyUser.hpp"
#include"../netio/Hook.hpp"

namespace zhuyh
{
namespace proxy
{
  void SecureHttpProxyServer::handleClient(Socket::ptr client)
  {
    auto session = makeSession(client);
    //LOG_ROOT_ERROR() << "client Nonb status : "<<(fcntl_f(client->getSockFd(),F_GETFL) & O_NONBLOCK);
    auto req = session->recvRequest();
    if(req == nullptr) return handleNotFound(session);
    //只处理CONNECT方法,其他方法视为恶意请求
    if(req->getMethod() != http::HttpMethod::CONNECT)
      {
	LOG_ROOT_ERROR() << "Invalid Request Method";
	return handleNotFound(session);
      }
    auto host = req->getHeader("Host","");
    LOG_ROOT_ERROR() << " host : " << host;
    if(host.empty()) return handleNotFound(session);
    auto connection = makeConnection(host,443);
    if(connection == nullptr) return handleNotFound(session,req->toString());

    //LOG_ROOT_ERROR() << "server Nonb status : "<<(fcntl_f(connection->getSocket()->getSockFd(),F_GETFL) & O_NONBLOCK);
    LOG_ROOT_ERROR() << "Https Connection Established";
    http::HttpResponse::ptr resp = std::make_shared<http::HttpResponse>();
    resp->setStatus(http::HttpStatus::OK);
    resp->setClose(false);
    int rt = session->sendResponse(resp);

    if(rt < 0)
      {
	LOG_ROOT_ERROR()<< " failed to send response, error : "<<strerror(errno)
			<<" , errno:"<<errno;
	return;
      }
    auto user = ProxyUser::Create(session,connection);
    user->start();

    std::shared_ptr<char> data(new char[4096],[](char* ptr){ delete [] ptr;});
    auto buff = data.get();    
    rt = 0;
    while((rt = session->read(buff,4096))>0)
      {
    	//LOG_ROOT_ERROR() << "Recv From CLIENT rt : " <<rt;
    	rt = connection->writeFixSize(buff,rt);
    	if(rt < 0)
    	  {
    	    LOG_ROOT_ERROR() << "failed to write to server, rt = " <<rt
    			     <<" error"<<strerror(errno)<<" errno : "<<errno;
    	    break;
    	  }
	co_yield;
      }
    //LOG_ROOT_ERROR() << "Handle Server EXITING";
  }
  
}
}
