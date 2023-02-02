#include "neon.h"

int main() {
  print_uint8x8_t(
    vqtbx4_u8(
      set_uint8x8_t(),
      set_uint8x16x4_t(),
      set_uint8x8_t()));
  return 0;
}
