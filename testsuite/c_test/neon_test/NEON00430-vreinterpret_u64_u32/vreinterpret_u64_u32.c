#include "neon.h"

int main() {
  print_uint64x1_t(
    vreinterpret_u64_u32(
      set_uint32x2_t()));
  return 0;
}
