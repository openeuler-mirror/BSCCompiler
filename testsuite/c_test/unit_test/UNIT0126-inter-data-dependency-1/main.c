#include "stdio.h"

int b = 7;

void func(int t)
{
  int i;
  int a = 3;
  int c = 64;
   
  if (t > 6) {
    a++;
  } else {
    c++;
  }  
 
  if (c == 65) {
    for (i = 0; i < 5000; ++i) {
       if (a <= 1200) {
         c = (c >> a) * b;
       }
       a = a + c;
    }
    printf("%d\n", c);
  }
  printf("%d\n", a);
}

int main()
{
  int tmp = 7;
  func((tmp << 2) + 5);
}
