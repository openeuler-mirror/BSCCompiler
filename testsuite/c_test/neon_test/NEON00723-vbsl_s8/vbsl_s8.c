#include "neon.h"

int main() {
  print_int8x8_t(
    vbsl_s8(
      set_uint8x8_t(),
      set_int8x8_t(),
      set_int8x8_t()));
  return 0;
}
