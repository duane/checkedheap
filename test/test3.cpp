#include <iostream>
using namespace std;

class Foo {
public:
  Foo (const char * i)
    : info (i)
  {}
  const char * info;
};


int main()
{
  Foo * f = new Foo ("happy");
  Foo * x = f;
  delete f;
  Foo * g = new Foo ("sad");
  
  // dang, dangling pointer
  cout << x->info << endl;
  return 0;
}
