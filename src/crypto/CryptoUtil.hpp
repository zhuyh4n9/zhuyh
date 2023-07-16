#pragma once

#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>

namespace zhuyh{
namespace crypto{
  std::string base64Encode(const char* buf,uint64_t buflen);
  std::string base64Decode(const char* buf,uint64_t buflen);

  std::string urlEncode(const std::string& str,bool spaceToPlus = true);
  std::string urlDecode(const std::string& str,bool spaceToPlus = true);
}
}
