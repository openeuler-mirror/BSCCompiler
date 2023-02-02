#include "neon.h"

int main() {
  print_int8x8_t(
    vqtbx3_s8(
      set_int8x8_t(),
      set_int8x16x3_t(),
      set_uint8x8_t()));
  return 0;
}
