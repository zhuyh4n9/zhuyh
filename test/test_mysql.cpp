#include "../zhuyh.hpp"
#include <iostream>
#include <mysql/mysql.h>
#include <exception>

const std::string prompt = "mysql>";
using namespace zhuyh;

const std::map<std::string,std::string>
params = {
	  {"user","root"},
	  {"passwd","169074291"},
	  {"host","127.0.0.1"},
	  {"port","3306"},
	  {"dbname","prac"}
};

void printDelimiter(int cols)
{
  std::cout<<"+";
  for(int i = 0;i<cols+1;i++)
    std::cout<<std::setfill('-')<< std::setw(13)<<"+"<<std::setfill(' ')<<std::setw(0);
  std::cout<<std::endl;
}

bool processRow(MYSQL_ROW row,int cols,int id)
{
  if(row == nullptr) return false;
  printf("|%12d|",id);
  for(int i=0;i<cols;i++)
    {
      printf("%12s|",row[i]);
    }
  printf("\n");
  printDelimiter(cols);
  return true;
}

void printHeader(int cols,db::MySQLRes::ptr res)
{
  printDelimiter(cols);
  printf("|%12s|","");
  for(int i = 0;i<cols;i++)
    printf("%12s|",res->getColumnName(i).c_str());
  putchar('\n');
  printDelimiter(cols);
  
}
void run()
{
  db::MySQLConn::ptr conn = std::make_shared<db::MySQLConn>(params);
  int rt = conn->connect();
  if(rt == false)
    {
      LOG_ROOT_ERROR() << "failed to connect to mysql server : "<<conn->getError();
      conn->close();
      return;
    }
  std::string sql;
  while(true)
    {
      std::cout<<prompt;
      getline(std::cin,sql);
      if(strncasecmp(sql.c_str(),"exit",4) == 0) break;
      db::MySQLRes::ptr res;
      try
	{
	  db::MySQLCommand::ptr cmd(new db::MySQLCommand(conn));
	  cmd->command(sql);
	  res = std::dynamic_pointer_cast<db::MySQLRes>(cmd->getRes());
	  if(res == nullptr) throw std::logic_error("cast failed");
	}
      catch(std::exception& e)
	{
	  LOG_ROOT_ERROR() << e.what();
	  break;
	}
      int cols = res->getColumnCount();
      printHeader(cols,res);
      res->foreach(processRow);
    }
  conn->close();
}

int main()
{
  co run;
  return 0;
}
