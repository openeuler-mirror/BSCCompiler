#include <stdio.h>

int main()
{
  int x = 1;
  goto Lab;
  x = x + 1;
  Lab: x = x + 5;
  printf("x = %d\n", x);
  return 0;
}
