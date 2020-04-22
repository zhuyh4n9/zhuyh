#include"SecureHttpProxyServer.hpp"
#include"ProxyUser.hpp"

namespace zhuyh
{
namespace proxy
{
  void SecureHttpProxyServer::handleClient(Socket::ptr client)
  {
    auto session = makeSession(client);
    auto req = session->recvRequest();
    if(req == nullptr) return handleNotFound(session);
    //只处理CONNECT方法,其他方法视为恶意请求
    if(req->getMethod() != http::HttpMethod::CONNECT)
      return handleNotFound(session);
    auto host = req->getHeader("Host","");
    LOG_ROOT_ERROR() << " host : " << host;
    if(host.empty()) return handleNotFound(session);
    auto connection = makeConnection(host,443);
    if(connection == nullptr) return handleNotFound(session,req->toString());

    LOG_ROOT_ERROR() << "Https Connection Established";
    http::HttpResponse::ptr resp = std::make_shared<http::HttpResponse>();
    resp->setStatus(http::HttpStatus::OK);
    resp->setClose(false);
    session->sendResponse(resp);
    
    auto user = ProxyUser::Create(session,connection);
    user->start();
  }
  
}
}
