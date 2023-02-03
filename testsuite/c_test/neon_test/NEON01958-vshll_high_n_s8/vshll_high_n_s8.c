#include "neon.h"

int main() {
  print_int16x8_t(
    vshll_high_n_s8(
      set_int8x16_t(),
      1));
  return 0;
}
