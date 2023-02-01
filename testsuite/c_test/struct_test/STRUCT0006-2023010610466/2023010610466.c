#include <stdio.h>

#pragma pack(push)
#pragma pack(1)
struct S0 {
  unsigned int f0;
  unsigned f1 : 12;
  signed f2 : 16;
  const volatile signed f3 : 18;
  unsigned f4 : 15;
  volatile unsigned f5 : 31;
  signed f6 : 4;
  const volatile unsigned f7 : 13;
};
#pragma pack(pop)

static struct S0 g_63 = {18446744073709551606UL, 60, -10, -354, 137, 11445, 2, 42}; /* VOLATILE GLOBAL g_63 */

/* ---------------------------------------- */
int main(int argc, char* argv[]) {
  printf("%d\n", g_63.f0);
  printf("%d\n", g_63.f1);
  printf("%d\n", g_63.f2);
  printf("%d\n", g_63.f3);
  return 0;
}