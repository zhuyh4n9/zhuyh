#pragma once

#include<memory>
#include<functional>
#include<vector>
#include<string>
#include<tinyxml2.h>
#include"../Singleton.hpp"
#include<fstream>
#include<unordered_map>
#include"Serializer.hpp"

//获取类偏移
#define OffsetOf(C,MV) ((size_t) &(static_cast<C*>(nullptr)->MV))

namespace zhuyh
{
namespace reflection
{
  
  //基本类型
  enum class MemberType
    {
     //有符号整型
     INT8,
     INT16,
     INT32,
     INT64,
     //无符号整型
     UINT8,
     UINT16,
     UINT32,
     UINT64,
     //浮点类型
     FLOAT,
     DOUBLE,
     //String类型
     STRING,
     //复合类型
     OBJECT,
     //未知类型
     UNKNOWN
    };
  //类成员
  class Member
  {
  public:
    typedef std::shared_ptr<Member> ptr;
    Member(const std::string& name,
	   const std::string& type,
	   bool pvt = true)
      :m_name(name),
       m_type(type),
       m_private(pvt),
       m_offset(-1)
    {
    }
    const std::string& getName() const  { return m_name;}
    const size_t&      getOffset() const { return m_offset;}
    void setOffset(size_t off) { m_offset = off; }
    const std::string&  getType() const {return m_type;}
    const bool getPrivate() const { return m_private;}
    //产生get方法
    const void generateGet(std::ostream& os);
    const void generateSet(std::ostream& os);
  private:
    Member& operator=(const Member&) = delete;
    Member(const Member& ) = delete;
  private:
    std::string m_name;
    std::string m_type;
    //是否为私有
    bool m_private;
    size_t m_offset;
    bool m_container;
  };
  //类型定义
  class ObjectDefine
  {
  public:
    typedef std::shared_ptr<ObjectDefine> ptr;
    ObjectDefine(std::string& name)
      :m_name(name)
    {}
    const std::vector<Member::ptr>& getAllMembers() const
    {
      return m_members;
    }
    const std::string& getName() const
    {
      return m_name;
    }
    void addMember(Member::ptr member)
    {
      m_members.push_back(member);
    }
    //类声明完整代码
    const void generate(std::ostream& os,bool newfile = true);
  private:
    //生产成员变量声明代码
    void generateVariable(std::ostream& os) const;
    //生成所有get和set方法代码
    void generateMethods(std::ostream& os) const;
    //生产序列化对象代码
    void generateSerialize(std::ostream& os) const;
    void generateDeSerialize(std::ostream& os) const;
    //初始化
    void generateInit(std::ostream& os) const;
  private:
    std::string m_name;
    std::vector<Member::ptr> m_members;
  };

  //类管理器，负责管理自定义类型映射
  class ObjectManager
  {
  public:
    friend class Singleton<ObjectManager>;
    friend class Member;
    friend class ObjectDefine;

    using CbType = std::function<bool(ObjectDefine::ptr,void*)>;
    bool addObj(const std::string& name,
		ObjectDefine::ptr obj,
		bool force = false)
    {
      auto it = m_objs.find(name);
      if(it != m_objs.end() && !force)
	{
	  return false;
	}
      m_objs[name] = obj;
      return true;
    }
    //从文件中读取类
    bool loadClass(const char* xmlpath);
    
    ObjectDefine::ptr getObj(const std::string& name) const
    {
      auto it = m_objs.find(name);
      if(it == m_objs.end())
	return nullptr;
      return it->second;
    }
    void generate(const std::string& name,std::ostream& os,bool newfile = true) const;

    bool foreach(CbType cb,void* arg)
    {
      for(auto& item: m_objs)
	{
	  if(cb(item.second,arg) == false) return false;
	}
      return true;
    }
  private:
    bool loadClass(tinyxml2::XMLElement* root);
  private:
    ObjectManager() {}
    ObjectManager(const ObjectManager&) = delete;
    ObjectManager& operator=(const ObjectManager&) = delete;
  private:
    std::unordered_map<std::string,ObjectDefine::ptr> m_objs;
  };
  using ObjMgr = Singleton<ObjectManager>;
  
}
}
