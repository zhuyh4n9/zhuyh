#pragma once

#include "../macro.hpp"

namespace zhuyh
{
  class StackTrait
  {
  public:
    //获取保护页的数量
    static size_t& getProtectStackPageSize();
    static bool protectStack(void* stack,size_t size,
			     size_t  pages_protect);
    static void unprotectStack(void* stack,size_t pages_protect);
  };
}
