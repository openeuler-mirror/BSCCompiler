#include "neon.h"

int main() {
  print_int8x8_t(
    vqtbl2_s8(
      set_int8x16x2_t(),
      set_uint8x8_t()));
  return 0;
}
