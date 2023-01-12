#include "neon.h"

int main() {
  print_int16x8_t(
    vreinterpretq_s16_s32(
      set_int32x4_t()));
  return 0;
}
