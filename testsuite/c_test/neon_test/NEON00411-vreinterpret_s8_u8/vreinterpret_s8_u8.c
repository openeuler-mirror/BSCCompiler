#include "neon.h"

int main() {
  print_int8x8_t(
    vreinterpret_s8_u8(
      set_uint8x8_t()));
  return 0;
}
