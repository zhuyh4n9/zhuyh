#pragma once

#include<openssl/sha.h>
#include<openssl/md5.h>
#include<openssl/pem.h>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>

namespace zhuyh{
namespace crypto{
  std::string sha1(const char* buf,uint64_t buflen = 0);
  std::string sha256(const char* buf,uint64_t buflen = 0);
  std::string md5(const char* buf,uint64_t buflen = 0);
  std::string base64Encode(const char* buf,uint64_t buflen);
  std::string base64Decode(const char* buf,uint64_t buflen);
}
}
