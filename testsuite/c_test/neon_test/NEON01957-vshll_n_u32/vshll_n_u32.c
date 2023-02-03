#include "neon.h"

int main() {
  print_uint64x2_t(
    vshll_n_u32(
      set_uint32x2_t(),
      1));
  return 0;
}
