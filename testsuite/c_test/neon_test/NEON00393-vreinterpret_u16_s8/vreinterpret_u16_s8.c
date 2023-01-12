#include "neon.h"

int main() {
  print_uint16x4_t(
    vreinterpret_u16_s8(
      set_int8x8_t()));
  return 0;
}
