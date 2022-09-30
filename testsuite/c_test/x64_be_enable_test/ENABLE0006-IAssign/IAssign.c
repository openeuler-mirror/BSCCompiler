#include <stdio.h>

int main()
{
  int x = 1;
  int *p = &x;
  *p = x + 5;
  printf("*p = %d\n", x);
  return 0;
}
