#include "neon.h"

int main() {
  print_uint32x4_t(
    vaddhn_high_u64(
      set_uint32x2_t(),
      set_uint64x2_t(),
      set_uint64x2_t()));
  return 0;
}
