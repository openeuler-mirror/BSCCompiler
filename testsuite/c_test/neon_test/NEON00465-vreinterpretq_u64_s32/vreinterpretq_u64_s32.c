#include "neon.h"

int main() {
  print_uint64x2_t(
    vreinterpretq_u64_s32(
      set_int32x4_t()));
  return 0;
}
