#pragma once

#include<byteswap.h>
#include<stdint.h>


namespace zhuyh
{
  template<class T>
  typename std::enable_if< sizeof(T) == sizeof(uint64_t),T>::type
  byteSwap(T value)
  {
    return (T)bswap_64((uint64_t)value);
  }
  
  template<class T>
  typename std::enable_if< sizeof(T) == sizeof(uint32_t),T>::type
  byteSwap(T value)
  {
    return (T)bswap_32((uint32_t)value);
  }

  template<class T>
  typename std::enable_if< sizeof(T) == sizeof(uint16_t),T>::type
  byteSwap(T value)
  {
    return (T)bswap_16((uint16_t)value);
  }
  
  
#if BYTE_ORDER == BIG_ENDIAN
  template<class T>
  T byteSwapOnLittleEndian(T t)
  {
    return t;
  }
  template<class T>
  T byteSwapOnBigEndian(T t)
  {
    return byteSwap(t);
  }
#else
  template<class T>
  T byteSwapOnLittleEndian(T t)
  {
    return byteSwap(t);
  }
  template<class T>
  T byteSwapOnBigEndian(T t)
  {
    return t;
  }
#endif
};
