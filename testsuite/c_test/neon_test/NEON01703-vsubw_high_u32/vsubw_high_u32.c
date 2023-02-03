#include "neon.h"

int main() {
  print_uint64x2_t(
    vsubw_high_u32(
      set_uint64x2_t(),
      set_uint32x4_t()));
  return 0;
}
