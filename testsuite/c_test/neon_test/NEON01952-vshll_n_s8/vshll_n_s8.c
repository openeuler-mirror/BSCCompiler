#include "neon.h"

int main() {
  print_int16x8_t(
    vshll_n_s8(
      set_int8x8_t(),
      1));
  return 0;
}
