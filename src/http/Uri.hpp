#pragma once

#include <memory>
#include <string>
#include <stdint.h>
#include "../socket/Address.hpp"

namespace zhuyh
{
namespace http
{
  class Uri
  {
  public:
    typedef std::shared_ptr<Uri> ptr;

    static newUri(const std::string& uri);

    IAddress::ptr newAddress();
    Uri();
    
    const std::string& getScheme() const { return m_scheme;}
    const std::string getUserinfo() const { return m_userinfo; }
    const std::string getHost() const { return m_host; }
    uint16_t  getPort() const { return m_port; }
    const std::string getPath() const { return m_path; }
    const std::string getQuery() const { return m_query; }
    const std::string getFragment() const { return m_fragment; }
    
    void setScheme(const std::string& v){  m_scheme = v;    }
    void setUserinfo(const std::string& v)    { m_userinfo = v;}
    void setHost(const std::string& v){ m_host = v;}
    void setPath(const std::string& v){ m_path = v;}
    void setPort(int v) { m_port = v;}
    void setQuery(const std::string& v){ m_Query = v;}
    void setFagment(const std::string& v){ m_fragment = v;}

    std::ostream& dump(std::ostream& os) const;
    std::string toString() const;
    IAddress::ptr newAddress();
    
  private:
    std::string m_scheme;
    std::string m_userinfo;
    std::string m_host;
    uint16_t m_port;
    std::string m_path;
    std::string m_query;
    std::string m_fragment;
  };
}
}
