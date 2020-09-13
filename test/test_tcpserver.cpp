#include<bits/stdc++.h>
#include"all.hpp"

using namespace zhuyh;

void run()
{
  IAddress::ptr addr = IAddress::newAddressByHostAny("0.0.0.0:8080");
  UnixAddress::ptr unix_addr(new UnixAddress("unix.sock"));
  LOG_ROOT_INFO() << *addr;
  LOG_ROOT_INFO() << *unix_addr;

  std::vector<IAddress::ptr> addrs;
  std::vector<IAddress::ptr> fails;
  addrs.push_back(addr);
  addrs.push_back(unix_addr);
  TcpServer::ptr server = std::make_shared<TcpServer>();
  //LOG_ROOT_INFO() << "HERE";
  while(!server->bind(addrs,fails))
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
