#include "neon.h"

int main() {
  print_uint64x2_t(
    vmlal_n_u32(
      set_uint64x2_t(),
      set_uint32x2_t(),
      set_uint32_t()));
  return 0;
}
