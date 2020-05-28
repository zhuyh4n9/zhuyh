#include"CryptoUtil.hpp"
#include<algorithm>
#include<string>
#include<cstring>
#include<iomanip>
#include<sstream>
using namespace boost::archive::iterators;

namespace zhuyh{
namespace crypto{
  
  uint32_t suffixZero(unsigned char* buf,int len){
    uint32_t res = 0;
    for(int i = len -1;i>=0;i-- ){
      if(buf[i]) return res;
      res++;
    }
    return res;
  }
  //为0认为是纯文本
  std::string sha1(const char* buf,uint64_t buflen){
    if(buf == nullptr) throw std::logic_error("buf is nullptr");
    if(buflen == 0){
      buflen = std::strlen(buf);
    }
    std::stringstream ss;
    unsigned char out[20] = {0};
    SHA1((unsigned char*)buf,buflen,out);
    size_t len = 20 - suffixZero(out,20);
    ss<<std::hex<<std::setw(2);
    for(size_t i = 0 ;i<len;i++)
      ss<<std::hex<<(uint32_t)out[i]<<std::setfill('0');
    return ss.str();
  }
  
  std::string md5(const char* buf,uint64_t buflen){
    if(buf == nullptr) throw std::logic_error("buf is nullptr");
    if(buflen == 0){
      buflen = std::strlen(buf);
    }
    unsigned char out[32] = {0};
    MD5((unsigned char*)buf,buflen,out);
    std::stringstream ss;
    ss<<std::hex<<std::setw(2);
    size_t len = 32 - suffixZero(out,32);
    for(size_t i = 0 ;i<len;i++)
      ss<<std::hex<<(uint32_t)out[i]<<std::setfill('0');
    return ss.str();
  }

  std::string sha256(const char* buf,uint64_t buflen){
    if(buf == nullptr) throw std::logic_error("buf is nullptr");
    if(buflen == 0){
      buflen = strlen(buf);
    }
    std::stringstream ss;
    unsigned char out[32] = {0};
    SHA256((unsigned char*)buf,buflen,out);
    size_t len = 32 - suffixZero(out,32);
    ss<<std::hex<<std::setw(2);
    for(size_t i = 0 ;i<len;i++)
      ss<<std::hex<<(uint32_t)out[i]<<std::setfill('0');
    return ss.str();
  }
  
  std::string base64Encode(const char* buf,uint64_t buflen){
    if(buf == nullptr) throw std::logic_error("buf is nullptr");
    if(buflen == 0){
      buflen = std::strlen(buf);
    }
    typedef base64_from_binary<transform_width<std::string::const_iterator, 6, 8>>
      Base64EncodeIterator;
    std::stringstream ss;
    try {
      std::copy(Base64EncodeIterator(buf),
		Base64EncodeIterator(buf+buflen),
		std::ostream_iterator<char>(ss));
    } catch (...) {
      return "";
  }
    size_t equal_count = (3 - buflen % 3) % 3;
    for ( size_t i = 0; i < equal_count; i++ ){
      ss<<"=";
    }
    return ss.str();
}
  
  std::string base64Decode(const char* buf,uint64_t buflen){
    if(buf == nullptr) throw std::logic_error("buf is nullptr");
    if(buflen == 0){
      buflen = std::strlen(buf);
    }
    typedef transform_width<binary_from_base64<std::string::const_iterator>, 8, 6>
      Base64DecodeIterator;
    std::stringstream ss;
    try {
      std::copy(Base64DecodeIterator(buf),
		Base64DecodeIterator(buf+buflen),
		std::ostream_iterator<char>(ss));
    } catch(...) {
      return "";
    }
    return ss.str();
  }
  
}
}
