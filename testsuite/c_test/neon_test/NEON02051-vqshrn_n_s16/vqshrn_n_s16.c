#include "neon.h"

int main() {
  print_int8x8_t(
    vqshrn_n_s16(
      set_int16x8_t(),
      1));
  return 0;
}
