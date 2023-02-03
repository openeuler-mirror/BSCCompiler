#include "neon.h"

int main() {
  print_uint8x8_t(
    vqshl_u8(
      set_uint8x8_t(),
      set_int8x8_t()));
  return 0;
}
