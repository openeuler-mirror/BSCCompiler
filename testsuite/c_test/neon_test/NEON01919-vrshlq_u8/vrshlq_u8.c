#include "neon.h"

int main() {
  print_uint8x16_t(
    vrshlq_u8(
      set_uint8x16_t(),
      set_int8x16_t()));
  return 0;
}
