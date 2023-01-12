#include "neon.h"

int main() {
  print_int16x4_t(
    vreinterpret_s16_u32(
      set_uint32x2_t()));
  return 0;
}
