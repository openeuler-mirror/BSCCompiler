#include "neon.h"

int main() {
  print_uint8x16_t(
    vqrshrun_high_n_s16(
      set_uint8x8_t(),
      set_int16x8_t(),
      1));
  return 0;
}
