#include"reflect.hpp"
#include<iostream>

namespace zhuyh
{
namespace reflection
{
  bool ObjectManager::loadClass(const char* path)
  {
    std::shared_ptr<tinyxml2::XMLDocument> doc(new tinyxml2::XMLDocument);
    if(tinyxml2::XML_SUCCESS !=  doc->LoadFile(path))
      {
	//std::cout<<"Open Failed"<<std::endl;
	return false;
      }
    tinyxml2::XMLElement* root = doc->RootElement();
    if(root == nullptr)
      {
	//std::cout<<"Root Failed"<<std::endl;
	return false;
      }
    if(loadClass(root) == false)
      {
	//std::cout<<"load Failed"<<std::endl;
	return false;
      }
    return true;
  }

  //
  bool ObjectManager::loadClass(tinyxml2::XMLElement* root)
  {
    tinyxml2::XMLElement* cls = nullptr;
    for(cls = root->FirstChildElement("class");cls != nullptr;
	cls = cls->NextSiblingElement())
      {
	const tinyxml2::XMLAttribute* classAttr = cls->FindAttribute("name");
	//创建一个自定义类型对象
	std::string className = classAttr->Value();
	ObjectDefine::ptr obj(new ObjectDefine(className));
	//TODO:打印错误日志
	if(classAttr == nullptr)
	  {
	    //std::cout<<" No Attr name\n";
	    return false;
	  }
	tinyxml2::XMLElement* member = nullptr;
	for(member = cls->FirstChildElement("member");member != nullptr;
	    member = member->NextSiblingElement() )
	  {
	    const tinyxml2::XMLAttribute* memberAttr = member->FindAttribute("name");
	    if(memberAttr == nullptr)
	      {
		//std::cout<<" No Attr name2\n";
		return false;
	      }
	    std::string varName = memberAttr->Value();
	    
	    memberAttr = member->FindAttribute("type");
	    if(memberAttr == nullptr)
	      {
		//std::cout<<" No Attr type\n";
		return false;
	      }
	    std::string varType = memberAttr->Value();
	    
	    memberAttr = member->FindAttribute("attr");
	    bool attr = true;
	    if(memberAttr != nullptr)
	      {
		std::string varAttr = memberAttr->Value();
		if(varAttr != "private" && varAttr != "public")
		  {
		    //  std::cout<<" No private or public\n";
		    return false;
		  }
	      }
	    //std::cout<<varName << " "<< varType << " " << attr<< std::endl;
	    Member::ptr mb (new Member(varName,varType,attr));
	    obj->addMember(mb);
	  }
	auto mgr = ObjMgr::getInstance();
	if(mgr->addObj(className,obj) == false)
	  {
	    std::cout<<"failed to add obj to mgr"<<std::endl;
	    return false;
	  }
      }
    return true;
  }
  const void Member::generateGet(std::ostream& os)
  {
    std::string name;
    if(strncmp(m_name.c_str(),"m_",2) == 0)
      {
	name = m_name.substr(2,m_name.size());
      }
    os << "  const "<<m_type << "& get_"<<name<<"() const\n";
    os << "  {\n";
    os << "    return "<<m_name<<";\n";
    os << "  }\n";
  }
  const void Member::generateSet(std::ostream& os)
  {
    std::string name;
    if(strncmp(m_name.c_str(),"m_",2) == 0)
      {
	name = m_name.substr(2,m_name.size());
      }
    os << "  void set_"<<name<<"( const "<<m_type<<"& v)\n";
    os << "  {\n";
    os << "    "<<m_name<<" = v;\n";
    os << "  }\n";
  }

  void ObjectDefine::generateMethods(std::ostream& os) const
  {
    for(auto& item : m_members)
      {
	item->generateGet(os);
	item->generateSet(os);
      }
  }

  void ObjectDefine::generateVariable(std::ostream& os) const
  {
    std::vector<Member::ptr> prv,pub;
    for(auto& item : m_members)
      {
	if(item->getPrivate()) prv.push_back(item);
	else pub.push_back(item);
      }

    if(!pub.empty())
      {
	os << "public:\n";
	for(auto& item : prv)
	  {
	    os<< "  "<<item->getType()<< " " << item->getName()<< ";\n";
	  }
      }
	
    if(!prv.empty())
      {
	os << "private:\n";
	for(auto& item : prv)
	  {
	    os<< "  "<<item->getType()<< " " << item->getName()<< ";\n";
	  }
      }
  }

  void ObjectDefine::generateInit(std::ostream& os) const
  {
    std::string prefix = "  ";
    os << prefix << "//初始化偏移\n";
  }
  
  void ObjectDefine::generateSerialize(std::ostream& os) const
  {
    std::string prefix = "  ";
    //TODO : 待修改，和ByteArray结合改为二进制数组，目前简单起见使用ostream
    os << prefix << "bool write(zhuyh::ByteArray::ptr ba) const\n";
    os << prefix << "{\n";
    {
      auto tmp = prefix;
      prefix += prefix;
      //对每个成员序列化
      
      for(auto& item : m_members)
	{
	    os << prefix << "Serialize<"<<item->getType()<<">::work("
	       << item->getName()<<", ba);\n";
	}
      prefix = tmp;
    }
    os << prefix << "}\n";
  }
  void ObjectDefine::generateDeSerialize(std::ostream& os) const
  {
    std::string prefix = "  ";
    //TODO : 待修改，和ByteArray结合改为二进制数组，目前简单起见使用ostream
    os << prefix << "bool read(zhuyh::ByteArray::ptr ba) const\n";
    os << prefix << "{\n";
    {
      auto tmp = prefix;
      prefix += prefix;
      //对每个成员序列化
      
      for(auto& item : m_members)
	{
	    os << prefix << "DeSerialize<"<<item->getType()<<">::work("
	       << item->getName()<<", ba);\n";
	}
      prefix = tmp;
    }
    os << prefix << "}\n";
  }
  const void ObjectDefine::generate(std::ostream& os,bool newfile)
  {
    //新文件
    if(newfile)
      {
	os << "#pragma once\n";
	os << "#include<memory>\n";
	os << "#include<string>\n";
	os << "#include<functional>\n";
	os << "#include\"bytearray/ByteArray.hpp\"";
	os << "//序列化对象文件\n";
	os << "#include\"Serializer.hpp\"\n";    
	os << "\n";
	os << "//计算类内部偏移宏\n";
	os << "#define OffsetOf(C,MV) ((size_t) &(static_cast<C*>(nullptr)->MV))\n\n";
      }
    
    os <<"class "<<m_name<<"\n";
    os <<"{\n";
    os<<"public:\n";
    os <<"  typedef std::shared_ptr<"<<m_name<<"> ptr\n";
    generateMethods(os);
    generateSerialize(os);
    generateDeSerialize(os);
    generateVariable(os);
    os<<"};\n";
  }

  void ObjectManager::generate(const std::string& name,std::ostream& os,bool newfile) const
  {
    auto it =  m_objs.find(name);
    if(it == m_objs.end()) return;
    it->second->generate(os,newfile);
  }
  
}
}
