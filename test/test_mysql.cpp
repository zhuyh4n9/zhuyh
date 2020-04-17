#include "../zhuyh.hpp"
#include <iostream>
#include <mysql/mysql.h>
#include <exception>

const std::string prompt = "mysql>";
using namespace zhuyh;

void printDelimiter(int cols)
{
  std::cout<<"+";
  for(int i = 0;i<cols+1;i++)
    std::cout<<std::setfill('-')<< std::setw(21)<<"+"<<std::setfill(' ')<<std::setw(0);
  std::cout<<std::endl;
}

bool processRow(MYSQL_ROW row,int cols,int id,unsigned long* lens)
{
  if(row == nullptr) return false;
  std::cout<<'|'<<std::setw(20)<<id<<std::setw(0)<<'|';
  for(int i=0;i<cols;i++)
    {
      
      std::cout<< std::setw(20)<<std::string(row[i],lens[i])
	       <<std::setw(0)<<'|';
    }
  std::cout<<std::endl;
  printDelimiter(cols);
  return true;
}

void printHeader(int cols,db::MySQLRes::ptr res)
{
  printDelimiter(cols);
  std::cout<<'|'<<std::setw(20)<<' '<<std::setw(0)<<'|';
  for(int i = 0;i<cols;i++)
    std::cout<< std::setw(20)<<res->getColumnName(i)<<std::setw(0)<<'|';
  std::cout<<std::endl;
  printDelimiter(cols);
  
}
void run()
{
  db::MySQLConn::ptr conn = db::MySQLConn::Create("root");
  db::MySQLConnGuard cg(conn);
  if(conn == nullptr)
    {
      LOG_ROOT_ERROR() << "failed to create mysql connection";
      return;
    }
  std::string sql;
  while(true)
    {
      std::cout<<prompt;
      getline(std::cin,sql);
      if(strncasecmp(sql.c_str(),"exit",4) == 0) break;
      db::MySQLRes::ptr res = nullptr;
      try
	{
	  db::MySQLCommand::ptr cmd(new db::MySQLCommand(conn));
	  auto tmp = cmd->command(sql);
	  if(tmp)
	    {
	      res = std::dynamic_pointer_cast<db::MySQLRes>(tmp);
	      if(res == nullptr) throw std::logic_error("cast failed");
	    }
	}
      catch(std::exception& e)
	{
	  LOG_ROOT_ERROR() << e.what();
	  break;
	}
      if(res)
	{
	  int cols = res->getColumnCount();
	  printHeader(cols,res);
	  res->foreach(processRow);
	}
    }
}

int main()
{
  co run;
  return 0;
}
