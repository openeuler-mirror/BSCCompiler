#include "stdint.h"
union U2 {
   int8_t  f0;
   volatile int16_t  f1;
   uint32_t  f2;
   volatile uint16_t  f3;
};

static volatile union U2 g_255;

int main() {
  for (g_255.f3 = 0; g_255.f3 < 2; g_255.f3 += 1)
  {
      printf("111\n");
  }
  return 0;
}
