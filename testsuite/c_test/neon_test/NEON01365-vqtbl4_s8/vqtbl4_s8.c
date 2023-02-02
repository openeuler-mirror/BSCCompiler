#include "neon.h"

int main() {
  print_int8x8_t(
    vqtbl4_s8(
      set_int8x16x4_t(),
      set_uint8x8_t()));
  return 0;
}
