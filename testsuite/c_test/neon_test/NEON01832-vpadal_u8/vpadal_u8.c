#include "neon.h"

int main() {
  print_uint16x4_t(
    vpadal_u8(
      set_uint16x4_t(),
      set_uint8x8_t()));
  return 0;
}
