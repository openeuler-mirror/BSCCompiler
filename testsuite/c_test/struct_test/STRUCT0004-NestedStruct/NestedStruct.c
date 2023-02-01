#include <stdio.h>
#include <stdint.h>

struct S0 {
  uint32_t f0;
  int32_t f1;
  volatile uint32_t f2;
  uint64_t f3;
  int32_t f4;
  signed f5 : 6;
};

struct S1 {
  signed f0 : 3;
  volatile signed f1 : 21;
  signed f2 : 26;
  unsigned f3 : 1;
  signed f4 : 26;
  unsigned f5 : 29;
  volatile unsigned f6 : 21;
  unsigned f7 : 14;
  const signed f8 : 29;
  signed f9 : 23;
};

union U2 {
  const int32_t f0;
};

union U3 {
  uint8_t f0;
  volatile uint32_t f1;
  uint32_t f2;
  volatile int32_t f3;
  int64_t f4;
};

union U4 {
  uint32_t f0;
  uint32_t f1;
  volatile uint64_t f2;
  int64_t f3;
};

union U5 {
   int64_t f0;
   const int16_t f1;
   int8_t f2;
};

struct S6 {
  struct S0 f0;
  union U2 f1;
  union U3 f2;
  struct S1 f3;
  union U4 f4;
  union U5 f5;
  int8_t f6 : 6;
  uint16_t : 0;
  int32_t f7 : 25;
};

static struct S6 g_295 = {
  .f0 = {1UL, 0x97B9FFBL, 3709551612UL, 0x48B7552F0C8C580BLL, 6L, -3},
  .f1 = 9L,
  .f2.f2 = 0x9DL,
  .f3 = {0, 1427, 1732, 0, -2186, 13939, 1439, 69, 1978, 2689},
  .f4 = 551611UL,
  .f5.f0 = 0xDDF25E8L,
  .f6 = 5L,
  .f7 = 0xDC200BL
};

int main() {
  printf("g_295.f0.f1 = %d, g_295.f5.f0 = %ld, g_295.f7 = %d\n", g_295.f0.f1, g_295.f5.f0, g_295.f7);
  return 0;
}