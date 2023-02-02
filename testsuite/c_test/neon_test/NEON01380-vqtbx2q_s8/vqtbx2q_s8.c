#include "neon.h"

int main() {
  print_int8x16_t(
    vqtbx2q_s8(
      set_int8x16_t(),
      set_int8x16x2_t(),
      set_uint8x16_t()));
  return 0;
}
