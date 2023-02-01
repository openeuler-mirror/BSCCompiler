#include <stdio.h>

#pragma pack(1)
struct S0 {
  unsigned f1 : 15;
  signed f2 : 21;
  unsigned f3 : 13;
  const unsigned f5 : 31;
};
static struct S0 a = {8, 1, 4, 34590};
int main() { printf("%d\n", a.f5); }