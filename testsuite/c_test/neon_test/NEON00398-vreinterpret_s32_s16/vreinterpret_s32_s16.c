#include "neon.h"

int main() {
  print_int32x2_t(
    vreinterpret_s32_s16(
      set_int16x4_t()));
  return 0;
}
