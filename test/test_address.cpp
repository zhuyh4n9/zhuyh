#include <bits/stdc++.h>
#include <unordered_map>
#include "../zhuyh.hpp"

using namespace zhuyh;

int main()
{
  typedef IAddress::InterfaceInfo InterfaceInfo;
  co [](){
    IPAddress::ptr addr = IPAddress::newAddress("www.baidu.com",8080);
    if(!addr)
      {
	LOG_ROOT_ERROR()<<"failed";
	addr = IPAddress::newAddress("192.168.0.1",8080);
	if(!addr)
	  {
	    LOG_ROOT_ERROR()<<"Failed";
	    return;
	  }
      }
    IPAddress::ptr addr2 = addr->getSubnetMask(24);
    IPAddress::ptr addr3 = addr->getNetworkAddress(24);
    IPAddress::ptr addr4 = addr->getBroadcastAddress(24);
    IAddress::ptr addr5 = addr->clone();
    IPAddress::ptr addr6 = addr->getHostAddress(24);
    LOG_ROOT_INFO() << *addr;
    LOG_ROOT_INFO() << *addr2; 
    LOG_ROOT_INFO() << *addr3;
    LOG_ROOT_INFO() << *addr4;
    LOG_ROOT_INFO() << *addr5;
    LOG_ROOT_INFO() << *addr6;
  };
  co [](){
    std::vector<IAddress::ptr> res;
    if(IAddress::newAddressByHost(res,"www.baidu.com:http"))
      for(auto& item : res)
	{
	  LOG_ROOT_INFO()<<*item;
	}
    else
      LOG_ROOT_ERROR()<<"Failed";
    auto addr = IAddress::newAddressByHostAny("www.baidu.com");
    if(addr)
      LOG_ROOT_INFO()<<*addr;
    addr = IAddress::newAddressByHostAnyIp("www.baidu.com");
    if(addr)
      LOG_ROOT_INFO()<<*addr;
  };
  co [](){
    std::unordered_multimap<std::string,InterfaceInfo::ptr> res;
    if(IAddress::getInterfaceAddress(res))
      {
	for(auto& item : res)
	  {
	    LOG_ROOT_INFO()<<*(item.second);
	  }
      }
    std::vector<InterfaceInfo::ptr> res2;
    if(IAddress::getInterfaceAddress(res2,"wlp3s0"))
      {
	for(auto& item : res2)
	  {
	    LOG_ROOT_INFO()<<*(item);
	  }
      }
  };
  return 0;
}
