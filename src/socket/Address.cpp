#include "Address.hpp"
#include <sstream>
#include <stdint.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../logs.hpp"
#include <cstring>
#include <errno.h>
#include <netdb.h>

namespace zhuyh
{
  static Logger::ptr sys_log = GET_LOGGER("system");
  
  template<class T>
  typename std::enable_if< sizeof(T) == 1,T>::type
  getMaskCode(uint32_t bits)
  {
    return (T)( ((uint8_t)1 << (8 - bits) ) -(uint8_t)1);
  }
  template<class T>
  typename std::enable_if< sizeof(T) == 2,T>::type
  getMaskCode(uint32_t bits)
  {
    return (T)( ((uint16_t)1 << (16 - bits)) -(uint16_t)1);
  }
  template<class T>
  typename std::enable_if< sizeof(T) == 4,T>::type
  getMaskCode(uint32_t bits)
  {
    return (T)((1ul << (32 - bits)) -1ul);
  }
  template<class T>
  typename std::enable_if< sizeof(T) == 8,T>::type
  getMaskCode(uint32_t bits)
  {
    return (T)( (1ull << (64 - bits) ) - 1ul);
  }
  // template<T>
  // typename std::enable_if< sizeof(T) == 1 ||
  // 			   sizeof(T) == 2 ||
  // 			   sizeof(T) == 4 ||
  // 			   sizeof(T) == 8,T>::type
  // getMaskCode(uint32_t bits)
  // {
  //   return (T)( ((uint8_t)1 << sizeof(T)*8 - bits) -(uint8_t)1);
  // }
  int IAddress::getFamily() const
  {
    return getAddr()->sa_family;
  }

  std::string IAddress::toString() const
  {
    std::stringstream ss;
    display(ss);
    return ss.str();
  }

  bool IAddress::operator<(const IAddress& rhs) const
  {
    socklen_t len = std::min(getAddrLen(),rhs.getAddrLen());
    int res =  memcmp(getAddr(),rhs.getAddr(),len);
    if(res < 0) return true;
    else if(res > 0) return false;
    else if(getAddrLen() < rhs.getAddrLen()) return true;
    return false;
  }
  bool IAddress::operator>(const IAddress& rhs) const
  {
    return !(*this < rhs) && (*this != rhs);
  }
  bool IAddress::operator==(const IAddress& rhs) const
  {
    if(getAddrLen() != rhs.getAddrLen()) return false;
    socklen_t len = std::min(getAddrLen(),rhs.getAddrLen());
    int res =  memcmp(getAddr(),rhs.getAddr(),len);
    return res == 0;
  }
  bool IAddress::operator!=(const IAddress& rhs) const
  {
    return !(*this == rhs);
  }
  IAddress::ptr IAddress::newAddress(const sockaddr* addr)
  {
    if(addr == nullptr) return nullptr;
    switch(addr->sa_family)
      {
      case AF_INET:
	return std::make_shared<IPv4Address>(*(sockaddr_in*)addr);
      case AF_INET6:
	return std::make_shared<IPv6Address>(*(sockaddr_in6*)addr);
      default:
	return std::make_shared<UnknownAddress>(*addr);
      }
  }

  IAddress::ptr IAddress::newAddressByHostAny(const std::string& host,
					      int family ,int sockType ,int protocol)
  {
    std::vector<IAddress::ptr> res;
    if(newAddressByHost(res,host,1,family,sockType,protocol))
      {
	if(!res.empty())
	  return res[0];
      }
    return nullptr;
  }
  
  std::shared_ptr<IPAddress>
  IAddress::newAddressByHostAnyIp(const std::string& host,int family ,
				  int sockType,int protocol)
  {
    std::vector<IAddress::ptr> res;
    if(newAddressByHost(res,host,1,family,sockType,protocol))
      {
	if(res.empty())
	  return nullptr;
	IPAddress::ptr ipAddr;
	for(auto& item : res)
	  {
	    ipAddr = std::dynamic_pointer_cast<IPAddress>(item);
	    if(ipAddr)
	      return ipAddr;
	  }
      }
    return nullptr;
  }

  bool IAddress::newAddressByHost(std::vector<IAddress::ptr>& res,const std::string& host,
				  bool any,int family ,int sockType ,int protocol)
  {
    struct addrinfo hints, *addrRes = nullptr;
    memset(&hints,0,sizeof(hints));
    hints.ai_family = family;
    hints.ai_socktype = sockType;
    hints.ai_protocol = protocol;
    
    std::string node;
    const char* service = nullptr;
    //ipv6address service
    if(!host.empty() && host[0] == ']')
      {
	const char* pos = (const char*)memchr(host.c_str(),']',host.size());
	if(pos)
	  {
	    int iv = pos -  host.c_str();
	    if((uint32_t)(iv+2) < host.size() && *(pos+1) == ':')
	      {
		service = pos+2;
	      }
	    node = host.substr(1,pos - host.c_str() -1);
	  }
      }
	// node service
	if(node.empty())
	  {
	    service = (const char*)memchr(host.c_str(),':',host.size());
	    if(service)
	      {
		int iv = service - host.c_str();
		if((uint32_t)(iv+1) < host.size() )
		  {
		    node = host.substr(0,service-host.c_str());
		    service++;
		  }
		else
		  service = nullptr;
	      }
	  }
	if(node.empty())
	  {
	    node = host;
	  }
	int error = getaddrinfo(node.c_str(),service,&hints,&addrRes);
	if(error)
	  {
	    LOG_ERROR(sys_log) << "newAddressByHost("<<host<<","
			       <<family<<","
			       <<sockType<<","<<protocol<<") error = "<<gai_strerror(error)
			       <<"errno = "<<errno;
	    return false;
	  }
	auto nxt = addrRes;
	while(nxt)
	  {
	    res.push_back(newAddress(nxt->ai_addr));
	    if(any) break;
	    nxt = nxt->ai_next;
	  }
	freeaddrinfo(addrRes);
	return true;
  }
  //使用dns获取地址
  IPAddress::ptr IPAddress::newAddress(const std::string& addr,uint16_t port)
  {
    struct addrinfo hints,*res = nullptr;
    memset(&hints,0,sizeof(hints));
    //阻止域名解析
    hints.ai_flags = AI_NUMERICHOST;
    hints.ai_family = AF_UNSPEC;

    int error = getaddrinfo(addr.c_str(),NULL,&hints,&res);
    if(error == EAI_NONAME)
      {
	return nullptr;
      }
    else if(error)
      {
	LOG_ERROR(sys_log) << "IPAddress::newAddress("<<addr<<","<<port
			   <<") errno = "<<errno<<" error = "
			   <<gai_strerror(error);
	return nullptr;
      }
    IPAddress::ptr newAddr =
      std::dynamic_pointer_cast<IPAddress>(IAddress::newAddress(res->ai_addr));
    if(newAddr != nullptr)
      {
	newAddr->setPort(port);
      }
    freeaddrinfo(res);
    return newAddr;
  }
  //IPv4
  //点分十进制创建一个Address,不属于构造函数原因是地址可能不合法
  IPv4Address::ptr IPv4Address::newAddress(const std::string& addr,uint16_t port)
  {
    sockaddr_in newAddr;
    bzero(&newAddr,sizeof(newAddr));
    newAddr.sin_family = AF_INET;
    newAddr.sin_port =  byteSwapOnLittleEndian(port);
    if(inet_pton(AF_INET,addr.c_str(),&newAddr.sin_addr) != 1)
      {
	LOG_ERROR(sys_log) << "failed to create a new IPv4 Address : "<<addr
			   <<" is not valid , error = " << strerror(errno);
	return nullptr;
      }
  return std::make_shared<IPv4Address>(newAddr);
  }
  sockaddr* IPv4Address::getAddr() 
  {
    return (sockaddr*)&m_addr;
  }
  const sockaddr* IPv4Address::getAddr() const
  {
    return (sockaddr*)&m_addr;
  }
  socklen_t IPv4Address::getAddrLen() const
  {
    return sizeof(m_addr);
  }
  std::ostream& IPv4Address::display(std::ostream& os) const 
  {
    uint32_t addr = byteSwapOnLittleEndian(m_addr.sin_addr.s_addr);
    os << ( (addr >> 24) & 0xff) << "."
       << ( (addr >> 16) & 0xff) << "."
       << ( (addr >> 8)  & 0xff) << "."
       << ( addr         & 0xff);
    os << ":" << byteSwapOnLittleEndian(m_addr.sin_port);
    return os;
  }
  uint32_t IPv4Address::getPort() 
  {
    return byteSwapOnLittleEndian(m_addr.sin_port);
  }
  void IPv4Address::setPort(uint16_t port) 
  {
    m_addr.sin_port = byteSwapOnLittleEndian(port);
  }

  IPAddress::ptr IPv4Address::getBroadcastAddress(uint32_t prefix_len)
  {
    if(prefix_len > 32) return nullptr;
    sockaddr_in baddr{m_addr};
    baddr.sin_addr.s_addr |=
      byteSwapOnLittleEndian(getMaskCode<uint32_t>(prefix_len));
    return std::make_shared<IPv4Address>(baddr);
  }
  IPAddress::ptr IPv4Address::getNetworkAddress(uint32_t prefix_len)
  {
    if(prefix_len > 32) return nullptr;
    sockaddr_in naddr{m_addr};
    naddr.sin_addr.s_addr &=
      ~byteSwapOnLittleEndian(getMaskCode<uint32_t>(prefix_len));
    return std::make_shared<IPv4Address>(naddr);
  }
  IPAddress::ptr IPv4Address::getHostAddress(uint32_t prefix_len)
  {
    if(prefix_len > 32) return nullptr;
    sockaddr_in naddr{m_addr};
    naddr.sin_addr.s_addr &=
      byteSwapOnLittleEndian(getMaskCode<uint32_t>(prefix_len));
    return std::make_shared<IPv4Address>(naddr);
  }
  IPAddress::ptr IPv4Address::getSubnetMask(uint32_t prefix_len)
  {
    if(prefix_len > 32 ) return nullptr;
    sockaddr_in saddr;
    memset(&saddr,0,sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr =
      ~byteSwapOnLittleEndian(getMaskCode<uint32_t>(prefix_len));
    return std::make_shared<IPv4Address>(saddr);
  }

  //IPv6
  IPv6Address::ptr IPv6Address::newAddress(const std::string& addr,uint16_t port)
  {
    struct sockaddr_in6 newAddr;
    bzero(&newAddr,sizeof(newAddr));
    newAddr.sin6_family = AF_INET6;
    newAddr.sin6_port = byteSwapOnLittleEndian(port);
    if(inet_pton(AF_INET6,addr.c_str(),&newAddr.sin6_addr) != 1)
      {
	LOG_ERROR(sys_log) << "failed to create a new IPv6 Address : "<<addr
			   <<" is not valid";
	return nullptr;
      }
    return std::make_shared<IPv6Address>(newAddr);
  }
  sockaddr* IPv6Address::getAddr() 
  {
    return (sockaddr*)&m_addr;
  }
  const sockaddr* IPv6Address::getAddr() const
  {
    return (sockaddr*)&m_addr;
  }
  socklen_t IPv6Address::getAddrLen() const
  {
    return sizeof(m_addr);
  }
  std::ostream& IPv6Address::display(std::ostream& os) const 
  {
    os << "[";
    bool tag = false;
    uint16_t* addr = (uint16_t*)m_addr.sin6_addr.s6_addr;
    for(int i=0;i<8;i++)
      {
        if(addr[i] == 0 && !tag) continue;
	if(i != 0 && addr[i-1] == 0 && !tag)
	  {
	    tag = true;
	    os<<":";
	  }
	if(i) os<<":";
	os<< std::hex << byteSwapOnLittleEndian(addr[i]) << std::dec;
	  
      }
    //全0
    if(!tag && addr[7] == 0) os<<"::";
    os<<"]:"<<m_addr.sin6_port;
    return os;
  }
  uint32_t IPv6Address::getPort() 
  {
    return m_addr.sin6_port;
  }
  void IPv6Address::setPort(uint16_t port) 
  {
    m_addr.sin6_port = byteSwapOnLittleEndian(port);
  }
  IPAddress::ptr IPv6Address::getBroadcastAddress(uint32_t prefix_len)
  {
    if(prefix_len > 128) return nullptr;
    sockaddr_in6 baddr{m_addr};
    baddr.sin6_addr.s6_addr[prefix_len/8] |=
      getMaskCode<uint8_t>(prefix_len%8);
    for(auto i=prefix_len/8+1;i<16;i++)
      {
	baddr.sin6_addr.s6_addr[i] = 0xff;
      }
    return std::make_shared<IPv6Address>(baddr);
  }
  IPAddress::ptr IPv6Address::getNetworkAddress(uint32_t prefix_len )
  {
    if(prefix_len > 128) return nullptr;
    sockaddr_in6 naddr{m_addr};
    naddr.sin6_addr.s6_addr[prefix_len/8] &=
      ~getMaskCode<uint8_t>(prefix_len%8);
    for(auto i=prefix_len/8+1;i<16;i++)
      {
	naddr.sin6_addr.s6_addr[i] = 0x00;
      }
    return std::make_shared<IPv6Address>(naddr);
  }
  IPAddress::ptr IPv6Address::getHostAddress(uint32_t prefix_len )
  {
    if(prefix_len > 128) return nullptr;
    sockaddr_in6 naddr{m_addr};
    naddr.sin6_addr.s6_addr[prefix_len/8] &=
      getMaskCode<uint8_t>(prefix_len%8);
    for(uint32_t i=0;i<prefix_len/8;i++)
      {
	naddr.sin6_addr.s6_addr[i] = 0x00;
      }
    return std::make_shared<IPv6Address>(naddr);
  }
  
  IPAddress::ptr IPv6Address::getSubnetMask(uint32_t prefix_len)
  {
    if(prefix_len > 128) return nullptr;
    sockaddr_in6 saddr;
    memset(&saddr,0,sizeof(saddr));
    saddr.sin6_family = AF_INET6;
    saddr.sin6_addr.s6_addr[prefix_len/8] =
      ~getMaskCode<uint8_t>(prefix_len%8);
    for(auto i=prefix_len/8+1;i<16;i++)
      {
	saddr.sin6_addr.s6_addr[i] = 0xff;
      }
    return std::make_shared<IPv6Address>(saddr);
  }
  
  static const int MAX_PATH_LEN = sizeof(((struct sockaddr_un*)0)->sun_path)-1;
  UnixAddress::UnixAddress()
  {
    memset(&m_addr,0,sizeof(m_addr));
    m_addr.sun_family = AF_UNIX;
    //获取sun_path的偏移地址宏
    m_len = offsetof(sockaddr_un,sun_path) + MAX_PATH_LEN;
  }
  
  UnixAddress::UnixAddress(const std::string& path)
  {
    memset(&m_addr,0,sizeof(m_addr));
    m_addr.sun_family = AF_UNIX;
    m_len = path.size()+1;
    if(!path.empty() && path[0] == '\0')
      --m_len;
    if(m_len > sizeof(m_addr.sun_path))
      {
	throw std::logic_error("path is too long");
      }
    memcpy(m_addr.sun_path,path.c_str(),m_len);
    m_len+= offsetof(sockaddr_un,sun_path);
  }
  
  sockaddr* UnixAddress::getAddr() 
  {
    return (sockaddr*)&m_addr;
  }
  const sockaddr* UnixAddress::getAddr() const
  {
    return (sockaddr*)&m_addr;
  }
  socklen_t UnixAddress::getAddrLen()  const
  {
    return m_len;
  }
  std::ostream& UnixAddress::display(std::ostream& os) const 
  {
    if(m_len > offsetof(sockaddr_un,sun_path) &&
       m_addr.sun_path[0] == '\0')
      {
	os << "\\0" << std::string(m_addr.sun_path+1,
				   m_len - (offsetof(sockaddr_un,sun_path)+1));
      }
    else
      {
	os << m_addr.sun_path;
      }
    return os;
  }

  UnknownAddress::UnknownAddress(int family)
  {
    memset(&m_addr,0,sizeof(m_addr));
    m_addr.sa_family = family;
  }
  UnknownAddress::UnknownAddress(const sockaddr& addr)
    :m_addr(addr)
  {
  }
  sockaddr* UnknownAddress::getAddr() 
  {
    return &m_addr;
  }
  const sockaddr* UnknownAddress::getAddr() const
  {
    return &m_addr;
  }
  socklen_t UnknownAddress::getAddrLen() const
  {
    return sizeof(m_addr);
  }
  std::ostream& UnknownAddress::display(std::ostream& os) const 
  {
    os << "[Unknown Address] : family = "<< m_addr.sa_family;
    return os;
  }

  std::ostream& operator<<(std::ostream& os,const IAddress& addr)
  {
    addr.display(os);
    return os;
  }
}
