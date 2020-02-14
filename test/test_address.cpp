#include <bits/stdc++.h>
#include <unordered_map>
#include "../zhuyh.hpp"

using namespace zhuyh;

int main()
{
  typedef IAddress::InterfaceInfo InterfaceInfo;
  IPAddress::ptr addr = IPAddress::newAddress("192.168.1.1",8080);
  IPAddress::ptr addr2 = addr->getSubnetMask(24);
  IPAddress::ptr addr3 = addr->getNetworkAddress(24);
  IPAddress::ptr addr4 = addr->getBroadcastAddress(24);
  IAddress::ptr addr5 = addr->clone();
  IPAddress::ptr addr6 = addr->getHostAddress(24);
  std::cout << *addr << std::endl;
  std::cout << *addr2 << std::endl;
  std::cout << *addr3 << std::endl;
  std::cout << *addr4 << std::endl;
  std::cout << *addr5 << std::endl;
  std::cout << *addr6 << std::endl;
  co [](){
    std::vector<IAddress::ptr> res;
    if(IAddress::newAddressByHost(res,"www.baidu.com:ftp"))
      for(auto& item : res)
	{
	  std::cout<<*item<<std::endl;
	}
    else
      std::cout<<"Failed"<<std::endl;
    auto addr = IAddress::newAddressByHostAny("www.baidu.com");
    if(addr)
      std::cout<<*addr<<std::endl;
    addr = IAddress::newAddressByHostAnyIp("www.baidu.com");
    if(addr)
      std::cout<<*addr<<std::endl;
  };
  co [](){
    std::unordered_multimap<std::string,InterfaceInfo::ptr> res;
    if(IAddress::getInterfaceAddress(res))
      {
	for(auto& item : res)
	  {
	    std::cout<<*(item.second)<<std::endl;
	  }
      }
    std::vector<InterfaceInfo::ptr> res2;
    if(IAddress::getInterfaceAddress(res2,"wlp3s0"))
      {
	for(auto& item : res2)
	  {
	    std::cout<<*(item)<<std::endl;
	  }
      }
  };
  return 0;
}
