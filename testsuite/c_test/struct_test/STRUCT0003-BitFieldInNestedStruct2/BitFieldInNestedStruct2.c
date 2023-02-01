#include <stdio.h>
#include <stdint.h>

#pragma pack(push)
#pragma pack(1)
struct S2 {
  unsigned f0 : 1;
  volatile signed f1 : 18;
  const signed f2 : 25;
  signed : 0;
  volatile int16_t f3;
  volatile uint32_t f4;
  volatile unsigned f5 : 5;
  signed f6 : 17;
};
#pragma pack(pop)

struct S3 {
  struct S2 f0;
  uint16_t f1;
  const uint64_t f2;
  int16_t f3;
  int8_t f4;
  struct S2 f5;
  const int8_t f6;
  volatile uint32_t f7;
  const volatile int32_t f8;
  const struct S2 f9;
};

static struct S3 g_295 = {
  {0, 167, 415, -1L, 429291UL, 1, -7},
  0x1923L,
  0x8348DC139FA897B0LL,
  2L,
  0x93L,
  {0, 473, 818, 0L, 0UL, 2, 95},
  0L,
  0x72E30379L,
  0x35B24F8AL,
  {0, -48, 1708, 0x7F8BL, 0x5D0B1L, 0, -74}
};

int main() {
  printf("g_295.f0.f5 = %d, g_295.f0.f6 = %d\n", g_295.f0.f5, g_295.f0.f6);
  return 0;
}