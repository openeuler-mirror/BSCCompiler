#include "neon.h"

int main() {
  print_uint8x8_t(
    vtbl4_u8(
      set_uint8x8x4_t(),
      set_uint8x8_t()));
  return 0;
}
