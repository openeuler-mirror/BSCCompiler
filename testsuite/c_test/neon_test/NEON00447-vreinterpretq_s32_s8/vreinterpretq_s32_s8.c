#include "neon.h"

int main() {
  print_int32x4_t(
    vreinterpretq_s32_s8(
      set_int8x16_t()));
  return 0;
}
