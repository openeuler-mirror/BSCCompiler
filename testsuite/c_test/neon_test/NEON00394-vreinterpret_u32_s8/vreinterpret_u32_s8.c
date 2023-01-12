#include "neon.h"

int main() {
  print_uint32x2_t(
    vreinterpret_u32_s8(
      set_int8x8_t()));
  return 0;
}
