#include "neon.h"

int main() {
  print_uint32x4_t(
    vrshrn_high_n_u64(
      set_uint32x2_t(),
      set_uint64x2_t(),
      1));
  return 0;
}
