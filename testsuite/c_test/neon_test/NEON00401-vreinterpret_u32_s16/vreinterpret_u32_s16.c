#include "neon.h"

int main() {
  print_uint32x2_t(
    vreinterpret_u32_s16(
      set_int16x4_t()));
  return 0;
}
