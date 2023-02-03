#include "neon.h"

int main() {
  print_int16x8_t(
    vqshrn_high_n_s32(
      set_int16x4_t(),
      set_int32x4_t(),
      1));
  return 0;
}
