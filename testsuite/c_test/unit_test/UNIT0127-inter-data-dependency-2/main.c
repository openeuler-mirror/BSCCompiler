#include "stdio.h"
#include "stdint.h"

int32_t g_8 = 0;

union U0 {
   int8_t  f0;
   int32_t  f1;
   unsigned f2 : 17;
   uint8_t  f3;
};
union U0 g_1145 = {0L};

static int64_t bar(int64_t si1, int64_t si2)
{
  return (
    ((si1 > 0) && (si2 > 0)) ||
    ((si1 > 0) && (si2 <= 0)) ||
    ((si1 <= 0) && (si1 < (INT64_MIN / si2)))
  ) ? (si1) : si1 * si2;
}

void func_1(void)
{
  for (int i = 1; i <= 3; ++i)
  {
    g_8 = 627889609;
    int rhs = &g_1145;
    int arg0 = 4714656 != rhs;
    int wtmp1 = bar(arg0, 0x6E239453D6C3245FLL);
    if (wtmp1 || 0xF078L) {
      g_8 = 29;
    }
    printf("%d\n", g_8);
  }
}

int main ()
{
  func_1();
  printf("g_8: %d\n", g_8);
  return 0;
}

