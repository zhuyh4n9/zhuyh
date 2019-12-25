#include "../zhuyh.hpp"
#include <iostream>

using namespace zhuyh;

struct Work
{
  int finTag = 0;
  int val;
};
volatile bool go = false;
TSQueue<Work> tsque;
void consume()
{
  int total = 0;
  while(go) ;
  int ntake;
  //int nreal;
  while(1)
    {
      int c =rand()%2;
      if(c)
	{
	  Work w;
	  tsque.pop_front(w);
	  total++;
	  //fprintf(stderr,"Comsuming Single Value : %d\n",w.val);
	  if(w.finTag)
	    {
	      printf("%d\n",total);
	      return;
	    }
	}
      else
	{
	  ntake = rand()%500+1;
	  std::list<Work> ls;
	   tsque.popk_front(ntake,ls);
	  //fprintf(stderr,"Try take %d Values Real : %d\n",ntake,nreal);
	   for(auto& v:ls)
	    {
	      total++;
	      ///fprintf(stderr,"Comsuming Value : %d\n",v.val);
	      if(v.finTag)
		{
		  printf("%d\n",total);
		  return;
		}
	    }
	}
    }
}

void product()
{
  while(go) ;
  int times = 1000;
  int nput;
  int total = 0;
  while(times--)
    {
      int c =rand()%2;
      if(c)
	{
	  Work w;
	  w.val = rand()%10;
	  // fprintf(stderr,"Product Single Value : %d\n",w.val);
	  tsque.push_back(std::move(w));
	  total++;
	}
      else
	{
	  nput = rand()%500+1;
	  std::list<Work> ls;
	  for(int i=0;i<nput;i++)
	    {
	      Work w;
	      w.val = rand()%10;
	      ls.push_back(w);
	      total++;
	    }
	  //fprintf(stderr,"product : %d values\n",nput);
	  //for(auto& v:ls)
	  //{
	      //fprintf(stderr,"Producting Value : %d\n",v.val);
	  // }
	  tsque.pushk_back(ls);
	}
    }
  Work w;
  w.finTag = 1;
  w.val = rand()%10;
  tsque.push_back(std::move(w));
  total++;
  //  fprintf(stderr,"Producting Value : %d\n",w.val);
  printf("%d\n",total);
}

int main()
{
  srand(time(0));
  Thread::ptr t1(new Thread(&consume,"consume"));
  Thread::ptr t2(new Thread(&product,"product"));
  go = true;
  t1->join();
  t2->join();
  return 0;
}
