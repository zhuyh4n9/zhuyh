#include<bits/stdc++.h>
#include"../zhuyh.hpp"
using namespace zhuyh;

void test_byteArray()
{
  ByteArray::ptr array(new ByteArray(1));
  int times = rand()%20;
  for(int i = 0;i<times;i++)
    {
      int v = rand();
      LOG_ROOT_INFO() << "Writing : "<<v;
      array->writeVint32(v);
    }
  array->setPosition(0);
  LOG_ROOT_INFO() << "Current Position : "<<array->getPosition();
  std::string res = array->dumpToHex();
  std::cout<<res<<std::endl;
  try
    {
      while(1)
	{
	  int val = array->readVint32();
	  LOG_ROOT_INFO() << "Reading : "<<val;
	}
    }
  catch(std::exception& e)
    {
      LOG_ROOT_INFO() << "Finished error : "<<e.what();
    }

  times = rand()%20;
  for(int i = 0;i<times;i++)
    {
      int v = rand();
      LOG_ROOT_INFO() << "Writing : "<<v;
      array->writeVint64(v);
    }
  array->setPosition(0);
  LOG_ROOT_INFO() << "Current Position : "<<array->getPosition();
  res = array->dumpToHex();
  std::cout<<res<<std::endl;
    try
    {
      while(1)
	{
	  int val = array->readVint64();
	  LOG_ROOT_INFO() << "Reading : "<<val;
	}
    }
  catch(std::exception& e)
    {
      LOG_ROOT_INFO() << "Finished error : "<<e.what();
    }
}

void test_file(const std::string& path)
{
  ByteArray::ptr array(new ByteArray(1));
  array->loadFromFile(path);
  array->setPosition(0);
  std::string file = array->dump();
  std::cout<<array->getReadableSize()<<std::endl;
  std::cout<<file<<std::endl;
  array->setPosition(0);
  std::string str = array->readStringU16();
  std::cout<<str<<std::endl;
  str = array->readStringU32();
  std::cout<<str<<std::endl;
  str = array->readStringU64();
  std::cout<<str<<std::endl;
}

void test_double()
{
  ByteArray::ptr array(new ByteArray(1));
  array->writeDouble(1.3213123);
  array->writeFloat(1.31231f);
  array->setPosition(0);
  std::cout<<array->readDouble()<<std::endl;
  std::cout<<array->readFloat()<<std::endl;
  array->setPosition(0);
  array->dumpToFile("file.txt");
  array->clear();
  array->loadFromFile("file.txt");
  array->setPosition(0);
  std::cout<<array->readDouble()<<std::endl;
  std::cout<<array->readFloat()<<std::endl;
}
void test_string()
{
  ByteArray::ptr array(new ByteArray(1));
  std::string str = "Hello World >>>>>>>>>>>>";
  array->writeStringU16(str);
  array->writeStringU32(str);
  array->writeStringU64(str);
  array->setPosition(0);
  array->dumpToFile("file.txt");
  str = array->readStringU16();
  std::cout<<str<<std::endl;
  str = array->readStringU32();
  std::cout<<str<<std::endl;
  str = array->readStringU64();
  std::cout<<str<<std::endl;
}
int main()
{
  test_byteArray();
  test_string();
  test_file("file.txt");
  test_double();
  return 0;
}
