#include "neon.h"

int main() {
  print_uint8x16_t(
    vqtbl3q_u8(
      set_uint8x16x3_t(),
      set_uint8x16_t()));
  return 0;
}
