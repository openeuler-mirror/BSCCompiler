#include "neon.h"

int main() {
  print_int32x4_t(
    vqrshrn_high_n_s64(
      set_int32x2_t(),
      set_int64x2_t(),
      1));
  return 0;
}
