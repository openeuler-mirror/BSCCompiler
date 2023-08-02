#include <stdio.h>

int main() {
  int a = 0;
  int *ptrA = &a;
  long long tmp = (long long)(ptrA);

  int *ptrB = (int*)(tmp);
  *ptrB = 10;
  if (a != 10)
    abort();
  printf("%d\n", a); // 10
  return 0;
}
