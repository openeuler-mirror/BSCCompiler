#include "neon.h"

int main() {
  print_uint8x16_t(
    vorrq_u8(
      set_uint8x16_t(),
      set_uint8x16_t()));
  return 0;
}
