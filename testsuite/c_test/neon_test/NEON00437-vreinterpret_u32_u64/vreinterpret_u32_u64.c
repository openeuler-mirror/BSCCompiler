#include "neon.h"

int main() {
  print_uint32x2_t(
    vreinterpret_u32_u64(
      set_uint64x1_t()));
  return 0;
}
