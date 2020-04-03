#pragma once

//类型映射

#include"reflect.hpp"
#include<map>
#include<unordered_map>

namespace zhuyh
{
namespace reflection
{
  //基础类型映射,(u)int8/16/32/64,double,float,string
  class TypeMapping
  {
  public:
    static MemberType getType(const std::string& name)
    {
      auto& mp = getNameToTypeMap();
      auto it = mp.find(name) ;
      if(it == mp.end())
	{
	  return MemberType::OBJECT;
	}
      return (MemberType)it->second;
    }
    
    const static std::string& getName(MemberType type)
    {
      auto& mp = getTypeToNameMap();
      auto it = mp.find((size_t)type) ;
      if(it == mp.end())
	{
	  return std::string("OBJECT");
	}
      return it->second;
    }
  private:
    //只读
    static const std::unordered_map<size_t,std::string>& getTypeToNameMap()
    {
      static const std::unordered_map<size_t,std::string> mp =
	{
	 {(size_t)MemberType::INT8, "int8"},
	 {(size_t)MemberType::INT16,"int16"},
	 {(size_t)MemberType::INT32,"int32"},
	 {(size_t)MemberType::INT64,"int64"},
	 {(size_t)MemberType::UINT8, "uint8"},
	 {(size_t)MemberType::UINT16,"uint16"},
	 {(size_t)MemberType::UINT32,"uint32"},
	 {(size_t)MemberType::UINT64,"uint64"},

	 {(size_t)MemberType::FLOAT,"float"},
	 {(size_t)MemberType::DOUBLE,"double"},
	 
	 {(size_t)MemberType::STRING,"string"}
	};
      return mp;
    }
    static const std::unordered_map<std::string,size_t>& getNameToTypeMap()
    {
      static const std::unordered_map<std::string,size_t> mp =
	{
	 {"int8_t", (size_t)MemberType::INT8},
	 {"char", (size_t)MemberType::INT8},
	 {"int16_t",(size_t)MemberType::INT16},
	 {"short",(size_t)MemberType::INT16},
	 {"int32_t",(size_t)MemberType::INT32},
	 {"int",(size_t)MemberType::INT32},
	 {"int64_t",(size_t)MemberType::INT64},
	 {"long long",(size_t)MemberType::INT64},
	 
	 {"uint8_t", (size_t)MemberType::UINT8},
	 {"unsigned char", (size_t)MemberType::INT8},
	 {"uint16_t",(size_t)MemberType::UINT16},
	 {"unsigned short", (size_t)MemberType::INT16},
	 {"uint32_t",(size_t)MemberType::UINT32},
	 {"unsigned", (size_t)MemberType::INT32},
	 {"unsigned int", (size_t)MemberType::INT32},
	 {"uint64_t",(size_t)MemberType::UINT64},
	 {"unsigned long long",(size_t)MemberType::UINT64},

	 {"float",(size_t)MemberType::FLOAT},
	 {"double",(size_t)MemberType::DOUBLE},

	 {"std::string",(size_t)MemberType::STRING}
	 
	 // {"std::map",(size_t)MemberType::MAP},
	 // {"std::vector",(size_t)MemberType::VECTOR},
	 // {"std::list",(size_t)MemberType::LIST},
	 // {"std::set",(size_t)MemberType::SET},
	 // {"std::unorderde_set",(size_t)MemberType::SET},
	 // {"std::unoderded_map",(size_t)MemberType::MAP},
	};
      return mp;
    }
  };
}
}
