#include<bits/stdc++.h>
#include"../zhuyh.hpp"
using namespace zhuyh;

void run()
{
  IAddress::ptr addr = IAddress::newAddressByHostAnyIp("ahut.gj.chaoxing.com:80");
  if(!addr)
    {
      LOG_ROOT_ERROR() << "get addr error";
      return;
    }
  Socket::ptr sock = Socket::newTCPSocket();
  bool rt = sock->connect(addr);
  if(!rt)
    {
      LOG_ROOT_ERROR() << "Connect " << *addr << " failed";
      return;
    }
  http::HttpConnection::ptr conn =
    std::make_shared<http::HttpConnection>(sock);
  http::HttpRequest::ptr req =
    std::make_shared<http::HttpRequest>();
  req->setHeader("Host","ahut.gj.chaoxing.com");
  req->setPath("/portal");
  std::cout << *req << std::endl;
  int rc = conn->sendRequest(req);
  if(rc < 0)
    {
      LOG_ROOT_ERROR() << "Failed to send request";
    }
  http::HttpResponse::ptr resp = conn->recvResponse();
  if(!resp)
    {
      LOG_ROOT_ERROR() << " Recv response error : "<<strerror(errno)
		       <<"addr :"<<*addr<<" Socket: " << sock;
    }
  if(resp)
    std::cout<< *resp << std::endl;
  std::fstream ofs;
  ofs.open("t.dat",std::ios::out | std::ios::trunc);
  ofs<<*resp;
}

int main()
{
  co run;
  return 0;
}
