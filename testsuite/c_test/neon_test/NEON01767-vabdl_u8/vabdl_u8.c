#include "neon.h"

int main() {
  print_uint16x8_t(
    vabdl_u8(
      set_uint8x8_t(),
      set_uint8x8_t()));
  return 0;
}
