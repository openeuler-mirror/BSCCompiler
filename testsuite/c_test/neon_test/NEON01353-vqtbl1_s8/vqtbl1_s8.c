#include "neon.h"

int main() {
  print_int8x8_t(
    vqtbl1_s8(
      set_int8x16_t(),
      set_uint8x8_t()));
  return 0;
}
