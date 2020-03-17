#include<bits/stdc++.h>
#include"../zhuyh.hpp"

using namespace zhuyh;

void run()
{
  http::HttpServer::ptr server = std::make_shared<http::HttpServer>();
  IAddress::ptr addr = IAddress::newAddressByHostAnyIp("0.0.0.0:8080");
  while(!server->bind(addr))
    {
      sleep(2);
    }
  server->start();
}

int main()
{
  co run;
  return 0;
}
