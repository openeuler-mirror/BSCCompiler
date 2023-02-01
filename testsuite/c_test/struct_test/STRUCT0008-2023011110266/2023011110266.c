#include <stdio.h>

#pragma pack(1)
struct S0 {
  signed f0 : 23;
  signed f1 : 6;
  volatile signed f2 : 18;
  const signed f3 : 22;
  unsigned f4 : 3;
  signed long f7;
} g_3611[4] = {7 - 2 - 5 - 1018, 2, 1, 2, 6};
static volatile struct S0 g_3626 = {3, 9 - 8 - 3, 3, 7, 5, 0};
int main() {
  int i;
  for (i = 0; i < 4; i++)
    printf("%d\n", g_3611[i].f0);
  printf("%d\n", g_3626.f0);
}