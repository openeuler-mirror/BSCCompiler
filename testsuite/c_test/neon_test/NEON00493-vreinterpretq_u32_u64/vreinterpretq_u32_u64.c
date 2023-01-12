#include "neon.h"

int main() {
  print_uint32x4_t(
    vreinterpretq_u32_u64(
      set_uint64x2_t()));
  return 0;
}
