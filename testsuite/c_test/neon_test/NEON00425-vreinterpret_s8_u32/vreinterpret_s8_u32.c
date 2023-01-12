#include "neon.h"

int main() {
  print_int8x8_t(
    vreinterpret_s8_u32(
      set_uint32x2_t()));
  return 0;
}
