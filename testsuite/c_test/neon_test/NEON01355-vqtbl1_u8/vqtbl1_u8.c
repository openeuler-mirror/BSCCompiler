#include "neon.h"

int main() {
  print_uint8x8_t(
    vqtbl1_u8(
      set_uint8x16_t(),
      set_uint8x8_t()));
  return 0;
}
