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
  //数字，大小写字母，$-_.+!*'()
  static char uri_valid[256] = {0};
  namespace {
    struct UrlCodeIniter{
      UrlCodeIniter(){
    for(char i='a';i<'z';i++) uri_valid[(size_t)i] = 1;
    for(char i='A';i<'Z';i++) uri_valid[(size_t)i] = 1;
    for(char i='0';i<'9';i++) uri_valid[(size_t)i] = 1;
    uri_valid[(uint32_t)'$']=1;
    uri_valid[(uint32_t)'-']=1;
    uri_valid[(uint32_t)'_']=1;
    uri_valid[(uint32_t)'.']=1;	
    uri_valid[(uint32_t)'+']=1;
    uri_valid[(uint32_t)'!']=1;
    uri_valid[(uint32_t)'*']=1;
    uri_valid[(uint32_t)'\'']=1;
    uri_valid[(uint32_t)'(']=1;
    uri_valid[(uint32_t)')']=1;
      }
    };
    static UrlCodeIniter s_url_initer;
  };
  
  std::string urlEncode(const std::string& str,bool spaceToPlus){
    static const char* hexStr = "0123456789ABCDEF";
    std::stringstream ss;
    for(size_t i=0;i<str.size();i++){
      char c = str[i];
      if(c==' ' && spaceToPlus){
    ss<<'+';
      }else if(!uri_valid[(size_t)c]){
    ss<<'%'<<hexStr[(size_t)(c>>4)] << hexStr[(size_t)(c & 0xf)];
      }else{
    ss<<c;
      } 
    }
    return ss.str();
  }

  static char hexToDec(char c){
    if(c >= '0' && c<='9') return c-'0';
    if(c >= 'a' && c<='f') return c-'a'+10;
    if(c >= 'A' && c<='F') return c-'A'+10;
    return (char)-1;
  }

  static bool isHex(char c){
    return (c>='0' && c<='9') || (c>='A' && c<='F') || (c >='a' && c<='f');
  }
  std::string urlDecode(const std::string& str,bool spaceToPlus){
    std::stringstream ss;
    for(size_t i = 0;i<str.size();i++){
      char c = str[i];
      if(c == '%' && i+2 < str.size()
     && isHex(str[i+1])
     && isHex(str[i+2])){
    ss<<(char)((hexToDec(str[i+1]) << 4) | hexToDec(str[i+2]));
    i+=2;
      } else if(c == ' ' && spaceToPlus){
    ss<<'+';
      } else {
    ss<<c;
      }
    }
    return ss.str();
  }
}
}
