#include <stdio.h>

int main()
{
  int *PintA[4];
  int **PPintA[2];
  int a = 1;
  int *ptr = &a;
  PintA[2] = ptr;
  PPintA[1] = PintA;
  *PPintA[1][2] = 3;
  int b = *PPintA[1][2];
  printf("%d\n", a);
  return 0;
}
