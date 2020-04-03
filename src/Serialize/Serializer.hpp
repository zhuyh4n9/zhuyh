#pragma once
#include"../bytearray/ByteArray.hpp"
#include<stdint.h>

//模板类型需要根据具体类型实现具体的序列化

namespace zhuyh
{
namespace reflection
{
  template<class T>
  class Serialize
  {
    static void work(T& value,zhuyh::ByteArray::ptr ba)
    {
    }
  };
  
  template<>
  class Serialize<int8_t>
  {
    static void work(const int8_t& v,zhuyh::ByteArray::ptr ba)
  {
    ba->writeInt8(v);
  }
};
  
  template<>
  class Serialize<int16_t>
  {
    static void work(const int16_t& v,zhuyh::ByteArray::ptr ba)
    {
      ba->writeInt16(v);
    }
  };
  
  template<>
  class Serialize<int32_t>
  {
    static void work(const int32_t& v,zhuyh::ByteArray::ptr ba)
    {
      ba->writeVint32(v);
    }
  };

  template<>
  class Serialize<int64_t>
  {
    static void work(const int64_t& v,zhuyh::ByteArray::ptr ba)
    {
      ba->writeVint64(v);
    }
  };

  template<>
  class Serialize<uint8_t>
  {
    static void work(const uint8_t& v,zhuyh::ByteArray::ptr ba)
    {
      ba->writeUint8(v);
    }
  };

  template<>
  class Serialize<uint16_t>
  {
    static void work(const uint16_t& v,zhuyh::ByteArray::ptr ba)
    {
      ba->writeUint16(v);
    }
  };
  
  template<>
  class Serialize<uint32_t>
  {
    static void work(const uint32_t& v,zhuyh::ByteArray::ptr ba)
    {
      ba->writeVuint32(v);
    }
  };

  template<>
  class Serialize<uint64_t>
  {
    static void work(const uint64_t& v,zhuyh::ByteArray::ptr ba)
    {
      ba->writeVuint64(v);
    }
  };


  template<>
  class Serialize<float>
  {
    static void work(const float& v,zhuyh::ByteArray::ptr ba)
    {
      ba->writeFloat(v);
    }
  };

  template<>
  class Serialize<double>
  {
    static void work(const double& v,zhuyh::ByteArray::ptr ba)
    {
      ba->writeDouble(v);
    }
  };

  template<>
  class Serialize<std::string>
  {
    static void work(const std::string& v,zhuyh::ByteArray::ptr ba)
    {
      ba->writeStringU32(v);
    }
  };


  template<class T>
  class DeSerialize
  {
    static void work(const T& v,zhuyh::ByteArray::ptr ba)
    {
    }
  };

  template<>
  class DeSerialize<int8_t>
  {
    static void work(int8_t& v,zhuyh::ByteArray::ptr ba)
    {
      v = ba->readInt8();
    }
  };

  template<>
  class DeSerialize<int16_t>
  {
    static void work(int16_t& v,zhuyh::ByteArray::ptr ba)
    {
      v = ba->readInt16();
    }
  };

  template<>
  class DeSerialize<int32_t>
  {
    static void work(int32_t& v,zhuyh::ByteArray::ptr ba)
    {
      v = ba->readVint32();
    }
  };
  
  template<>
  class DeSerialize<int64_t>
  {
    static void work(int64_t& v,zhuyh::ByteArray::ptr ba)
    {
      v = ba->readVint64();
    }
  };

  template<>
  class DeSerialize<uint8_t>
  {
    static void work(uint8_t& v,zhuyh::ByteArray::ptr ba)
    {
      v = ba->readUint8();
    }
  };

  template<>
  class DeSerialize<uint16_t>
  {
    static void work(uint16_t& v,zhuyh::ByteArray::ptr ba)
    {
      v = ba->readUint16();
    }
  };

  template<>
  class DeSerialize<uint32_t>
  {
    static void work(uint32_t& v,zhuyh::ByteArray::ptr ba)
    {
      v = ba->readVuint32();
    }
  };

  template<>
  class DeSerialize<uint64_t>
  {
    static void work(uint64_t& v,zhuyh::ByteArray::ptr ba)
    {
      v = ba->readVuint64();
    }
  };

  template<>
  class DeSerialize<double>
  {
    static void work(double& v,zhuyh::ByteArray::ptr ba)
    {
      v = ba->readDouble();
    }
  };

  template<>
  class DeSerialize<float>
  {
    static void work(float& v,zhuyh::ByteArray::ptr ba)
    {
      v = ba->readDouble();
    }
  };

  template<>
  class DeSerialize<std::string>
  {
    static void work(std::string& v,zhuyh::ByteArray::ptr ba)
    {
      v = ba->readStringU32();
    }
  };
  
}
}
