#include<bits/stdc++.h>
#include"reflect.hpp"

int main(int argc,char* argv[])
{
  if(argc != 2)
    {
      std::cerr<<"Usage : Serialize <xmlfile>\n";
      exit(1);
    }
  std::string xmlpath = argv[1];
  auto mgr = zhuyh::reflection::ObjMgr::getInstance();
  if(mgr->loadClass(xmlpath.c_str()) == false)
    std::cerr<<"failed to load xml file : "<<xmlpath<<std::endl;
  bool newfile = true;
  auto cb = [](zhuyh::reflection::ObjectDefine::ptr obj,void* arg)
	    {
	      bool newfile = *((bool*)arg);
	      obj->generate(std::cout,newfile);
	      newfile = false;
	      return true;
	    };
  mgr->foreach(cb,&newfile);
  return 0;
}
