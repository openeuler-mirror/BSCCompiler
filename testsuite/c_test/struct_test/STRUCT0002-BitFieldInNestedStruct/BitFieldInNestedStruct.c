#include <stdio.h>
#include <stdint.h>

#pragma pack(push)
#pragma pack(1)
struct S1 {
  unsigned f0 : 29;
  unsigned f1 : 10;
  volatile unsigned f2 : 27;
  unsigned : 0;
  volatile signed f3 : 8;
  const volatile uint8_t f4;
  volatile unsigned f5 : 27;
  signed f6 : 5;
  signed f7 : 2;
};
#pragma pack(pop)

struct S2 {
  uint32_t f0;
  struct S1 f1;
  const volatile int64_t f2;
  signed f3 : 30;
  const int16_t f4;
  const int32_t f5;
  const uint8_t f6;
};

/* VOLATILE GLOBAL g_10 */
static struct S2 g_10 = {4UL, {7874, 10, 6774, 8, 0x61L, 3805, -0, 1}, 0xA37C05799F9A51C3LL, 18896, -10L, -3L, 0x51L};

int main () {
  printf("f3: %d\n", g_10.f3);
  return 0;
}