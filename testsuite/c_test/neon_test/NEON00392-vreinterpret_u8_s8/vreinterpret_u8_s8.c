#include "neon.h"

int main() {
  print_uint8x8_t(
    vreinterpret_u8_s8(
      set_int8x8_t()));
  return 0;
}
