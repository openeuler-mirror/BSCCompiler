#include "neon.h"

int main() {
  print_uint8x16_t(
    vrev16q_u8(
      set_uint8x16_t()));
  return 0;
}
