#pragma once

namespace zhuyh
{
  class UnCopyable
  {
  public:
    UnCopyable() = default;
    ~UnCopyable() = default;
  private:
    //拷贝构造，赋值是不被允许的
    UnCopyable(const UnCopyable& o) = delete ;
    UnCopyable& operator=(const UnCopyable& o) = delete;
  };
};
