#include "neon.h"

int main() {
  print_uint8x16_t(
    vdupq_n_u8(
      set_uint8_t()));
  return 0;
}
