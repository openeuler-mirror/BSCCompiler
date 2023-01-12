#include "neon.h"

int main() {
  print_int8x8_t(
    vreinterpret_s8_u16(
      set_uint16x4_t()));
  return 0;
}
