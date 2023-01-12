#include "neon.h"

int main() {
  print_int32x4_t(
    vreinterpretq_s32_u64(
      set_uint64x2_t()));
  return 0;
}
