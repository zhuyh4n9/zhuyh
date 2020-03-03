#pragma once

#include<unistd.h>
#include<memory>
#include<sys/socket.h>
#include<sys/types.h>
#include<atomic>
#include<cstring>
#include<sys/un.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include "endian.hpp"
#include <vector>
#include <unordered_map>
#include <iostream>

namespace zhuyh
{
  class IPAddress;
  class IAddress;
  std::ostream& operator<<(std::ostream& os,const IAddress& addr);
  class IAddress
  {
  public:
    typedef std::shared_ptr<IAddress> ptr;
    class InterfaceInfo
    {
    public:
      friend std::ostream& operator<<(std::ostream& os,const InterfaceInfo& addr);
      typedef std::shared_ptr<InterfaceInfo> ptr;
      InterfaceInfo(const std::string name,
		    IAddress::ptr addr,
		    uint32_t prefix_len)
	:m_name(name),m_addr(addr),m_prefix_len(prefix_len)
      {
      }
      const std::string& getIfName()
      {
	return m_name;
      }
      IAddress::ptr getAddress()
      {
	return m_addr;
      }
      uint32_t getPrefixLen()
      {
	return m_prefix_len;
      }
      std::ostream& display(std::ostream& os) const
      {
	os << "<"<<m_name<<","<<*m_addr<<","<<m_prefix_len<<">";
	return os;
      }
    private:
      std::string m_name;
      IAddress::ptr m_addr;
      uint32_t m_prefix_len;
    };
  public:
    virtual ~IAddress() {}
    virtual sockaddr* getAddr() = 0;
    virtual const sockaddr* getAddr() const = 0;
    virtual socklen_t getAddrLen() const = 0;
    virtual std::ostream& display(std::ostream& os) const = 0;
    std::string toString() const;
    int getFamily() const;
    virtual bool operator<(const IAddress& rhs) const;
    virtual bool operator>(const IAddress& rhs) const;
    virtual bool operator==(const IAddress& rhs) const;
    virtual bool operator!=(const IAddress& rhs) const;
    static  IAddress::ptr newAddress(const sockaddr* addr);
    virtual IAddress::ptr clone() = 0;
    static bool newAddressByHost(std::vector<IAddress::ptr>& res,const std::string& host,
				 bool any = false,int family = AF_UNSPEC,int sockType = 0,
				 int protocol = 0);
    //查找任意一个地址
    static IAddress::ptr newAddressByHostAny(const std::string& host,int family = AF_UNSPEC ,
					     int sockType = 0,int protocol = 0);
    //查找任意一个Ip地址
    static std::shared_ptr<IPAddress>
    newAddressByHostAnyIp(const std::string& host,
			  int family = AF_UNSPEC ,int sockType = 0,
			  int protocol = 0);
    static bool getInterfaceAddress(std::unordered_multimap<std::string,
				InterfaceInfo::ptr>& res,
				int family = AF_UNSPEC);
    static bool getInterfaceAddress(std::vector<InterfaceInfo::ptr>& res,
				    const std::string& name,
				    int family = AF_UNSPEC);
  };

  class IPAddress : public IAddress
  {
  public:
    typedef std::shared_ptr<IPAddress> ptr;
    virtual uint32_t getPort() = 0;
    virtual void setPort(uint16_t port) = 0;
    virtual IPAddress::ptr getBroadcastAddress(uint32_t prefix_len) = 0;
    virtual IPAddress::ptr getNetworkAddress(uint32_t prefix_len) = 0;
    virtual IPAddress::ptr getHostAddress(uint32_t prefix_len) = 0;
    virtual IPAddress::ptr getSubnetMask(uint32_t prefix_len) = 0;
    static  IPAddress::ptr newAddress(const std::string& addr,uint16_t port = 0);
  };

  class IPv4Address : public IPAddress
  {
  public:
    typedef std::shared_ptr<IPv4Address> ptr;
    IPv4Address()
    {
      bzero(&m_addr,sizeof(m_addr));
      m_addr.sin_family = AF_INET;
    }
    IPv4Address(uint32_t addr ,uint16_t port = 0 )
    {
      bzero(&m_addr,sizeof(m_addr));
      m_addr.sin_family = AF_INET;
      m_addr.sin_port = byteSwapOnLittleEndian(port);
      m_addr.sin_addr.s_addr = byteSwapOnLittleEndian(addr);
    }
    IPv4Address(const sockaddr_in& addr)
      :m_addr(addr)
    {
    }
    
    static IPv4Address::ptr newAddress(const std::string& addr,uint16_t port = 0);
    sockaddr* getAddr() override;
    const sockaddr* getAddr() const override;
    socklen_t getAddrLen() const override;
    std::ostream& display(std::ostream& os) const  override;
    uint32_t getPort() override;
    void setPort(uint16_t port) override;
    IPAddress::ptr getBroadcastAddress(uint32_t prefix_len) override;
    IPAddress::ptr getNetworkAddress(uint32_t prefix_len) override;
    IPAddress::ptr getHostAddress(uint32_t prefix_len) override;
    IPAddress::ptr getSubnetMask(uint32_t prefix_len) override;
    IAddress::ptr clone() override
    {
      return std::make_shared<IPv4Address>(*this);
    }
  private:
    struct sockaddr_in m_addr;
  };

  class IPv6Address : public IPAddress
  {
  public:
    typedef std::shared_ptr<IPv6Address> ptr;
    IPv6Address()
    {
      bzero(&m_addr,sizeof(m_addr));
      m_addr.sin6_family = AF_INET6;
    }
    IPv6Address(uint8_t addr[16],uint32_t port )
    {
      bzero(&m_addr,sizeof(m_addr));
      m_addr.sin6_family = AF_INET6;
      m_addr.sin6_port = byteSwapOnLittleEndian(port);
      memcpy(&m_addr.sin6_addr.s6_addr,addr,16);
    }
    IPv6Address(const sockaddr_in6& addr):m_addr(addr)
    {
    }
    
    static IPv6Address::ptr newAddress(const std::string& addr,uint16_t port = 0);
    sockaddr* getAddr() override;
    const sockaddr* getAddr() const override;
    socklen_t getAddrLen() const override;
    std::ostream& display(std::ostream& os) const override;
    uint32_t getPort() override;
    void setPort(uint16_t port) override;
    IPAddress::ptr getBroadcastAddress(uint32_t prefix_len) override;
    IPAddress::ptr getNetworkAddress(uint32_t prefix_len) override;
    IPAddress::ptr getHostAddress(uint32_t prefix_len) override;
    IPAddress::ptr getSubnetMask(uint32_t prefix_len) override;
    IAddress::ptr clone() override
    {
      return std::make_shared<IPv6Address>(*this);
    }
  private:
    struct sockaddr_in6 m_addr;
  };
  
  class UnixAddress : public IAddress
  {
  public:
    typedef std::shared_ptr<UnixAddress> ptr;
    UnixAddress();
    UnixAddress(const std::string& path);
    sockaddr* getAddr() override;
    const sockaddr* getAddr() const override;
    socklen_t getAddrLen() const override;
    std::ostream& display(std::ostream& os) const override;
    IAddress::ptr clone() override
    {
      return std::make_shared<UnixAddress>(*this);
    }
    void setAddrLen(socklen_t len)
    {
      m_len = len;
    }
  private:
    struct sockaddr_un m_addr;
    socklen_t m_len;
  };

  class UnknownAddress : public IAddress
  {
  public:
    typedef std::shared_ptr<UnknownAddress> ptr;
    UnknownAddress(int family);
    UnknownAddress(const sockaddr& addr);
    sockaddr* getAddr() override;
    const sockaddr* getAddr() const override;
    socklen_t getAddrLen() const override;
    std::ostream& display(std::ostream& os) const override;
    IAddress::ptr clone() override
    {
      return std::make_shared<UnknownAddress>(*this);
    }
  private:
    sockaddr m_addr;
  };
  std::ostream& operator<<(std::ostream& os,const IAddress::InterfaceInfo& addr);
}

