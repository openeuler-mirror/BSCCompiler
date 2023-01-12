#include "neon.h"

int main() {
  print_int32x2_t(
    vreinterpret_s32_u64(
      set_uint64x1_t()));
  return 0;
}
