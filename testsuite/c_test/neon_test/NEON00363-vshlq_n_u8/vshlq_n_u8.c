#include "neon.h"

int main() {
  print_uint8x16_t(
    vshlq_n_u8(
      set_uint8x16_t(),
      1));
  return 0;
}
