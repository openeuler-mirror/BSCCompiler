#include "neon.h"

int main() {
  print_int8x8_t(
    vqtbx1_s8(
      set_int8x8_t(),
      set_int8x16_t(),
      set_uint8x8_t()));
  return 0;
}
