#include "../zhuyh.hpp"

int main()
{
  for(int i = 0;i<2000;i++){
    co [](){
	 for(int i =0;i<50000;i++)
	   co_yield;
       };
  }
  //zhuyh::Scheduler::Schd::getInstance() -> stop();
  return 0;
}
