#include "neon.h"

int main() {
  print_uint8x16_t(
    vpminq_u8(
      set_uint8x16_t(),
      set_uint8x16_t()));
  return 0;
}
