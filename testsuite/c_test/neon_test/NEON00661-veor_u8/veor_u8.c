#include "neon.h"

int main() {
  print_uint8x8_t(
    veor_u8(
      set_uint8x8_t(),
      set_uint8x8_t()));
  return 0;
}
