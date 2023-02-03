#include "neon.h"

int main() {
  print_uint64x2_t(
    vaddl_high_u32(
      set_uint32x4_t(),
      set_uint32x4_t()));
  return 0;
}
