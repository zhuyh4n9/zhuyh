#include"config.hpp"

namespace zhuyh

{
  IConfigVar::ptr Config::lookUpBase(const std::string& varName)
  {
    auto& mp = getVarMap();
    auto it = mp.find(varName);
    return it == mp.end() ? nullptr : it->second;
  }

  bool Config::listAllItem(const std::string& prefix,
			   const YAML::Node& node,
			   std::list<std::pair<std::string,YAML::Node> >& out)
  {
    if(!prefix.empty() || prefix != "")
      out.push_back(std::make_pair(prefix,node) );
    if(node.IsMap() )
      {
	for(auto it = node.begin();
	    it != node.end(); it++)
	  {
	    std::string str = it->first.Scalar();
	    if(str.find_first_not_of("abcdefghijklmnopqrstuvwxyz._0123456789")
	       != std::string::npos)
	      {
		return false;
	      }
	    if(listAllItem(prefix.empty() ? str : prefix+"."+str,
			   it->second,out)  == false)
	      {
		return false;
	      }
	  }
      }
    return true;
  }

  bool Config::loadFromYamlNode(YAML::Node& node)
  {
    std::list<std::pair<std::string,YAML::Node> > out;
    if(listAllItem("",node,out) == false ) return false;
    LockGuard lg(lk());
    for(auto& val:out)
      {
	auto key = val.first;
	std::transform(key.begin(),key.end(),key.begin(),::tolower);
	auto var = lookUpBase(key);
	if(var)
	  {
	    if(val.second.IsScalar() )
	      {
		var->fromStr(val.second.Scalar() );
	      }
	    else
	      {
		std::stringstream ss;
		ss<<val.second;
		var->fromStr(ss.str() );
	      }
	  }
      }
    return true;
  }
  
  bool Config::loadFromYamlFile(const std::string& path,bool force)
  {
    try{
      YAML::Node root = YAML::LoadFile(path);
      loadFromYamlNode(root);
      return true;
    } catch (std::exception& e) {
      std::cout<<"Failed to LoadFile "<<path<<" error:"<<e.what()<<std::endl;
    }
    return false;
  }
  
}
  
