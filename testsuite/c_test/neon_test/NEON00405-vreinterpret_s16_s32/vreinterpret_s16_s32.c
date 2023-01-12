#include "neon.h"

int main() {
  print_int16x4_t(
    vreinterpret_s16_s32(
      set_int32x2_t()));
  return 0;
}
