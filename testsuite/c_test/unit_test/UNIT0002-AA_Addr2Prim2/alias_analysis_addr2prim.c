#include <stdio.h>

struct A {
  long long tmp;
} ;

int main() {
  int a = 0;
  int *ptrA = &a;
  struct A structA, *ptrStructA = &structA;

  ptrStructA->tmp = (long long)(ptrA);

  int *ptrB = (int*)(structA.tmp);
  *ptrB = 10;
  if (a != 10)
    abort();
  printf("%d\n", a); // 10
  return 0;
}
