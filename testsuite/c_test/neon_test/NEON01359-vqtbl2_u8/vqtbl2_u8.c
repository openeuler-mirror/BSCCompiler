#include "neon.h"

int main() {
  print_uint8x8_t(
    vqtbl2_u8(
      set_uint8x16x2_t(),
      set_uint8x8_t()));
  return 0;
}
