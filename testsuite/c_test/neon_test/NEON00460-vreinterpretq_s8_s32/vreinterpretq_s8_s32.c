#include "neon.h"

int main() {
  print_int8x16_t(
    vreinterpretq_s8_s32(
      set_int32x4_t()));
  return 0;
}
