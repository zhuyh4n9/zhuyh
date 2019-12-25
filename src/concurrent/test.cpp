#include <bits/stdc++.h>

using namespace std;


class A
{
public:
  typedef shared_ptr<A> ptr;
  A(int _id) :id(_id)  { cout<<"A Created!ID : "<<id<<endl;}
  ~A() { cout<<"A Destroyed!ID : "<<id<<endl;}
private:
  int id;
};

class B
{
public:
  typedef shared_ptr<B> ptr;
  B(A::ptr* _a)  { a.swap(*_a);}
private:
  A::ptr a;
};
int main()
{
  A::ptr a(new A(1));
  B::ptr b(new B(&a));
  return 0;
}
