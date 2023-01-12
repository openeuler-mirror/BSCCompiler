#include "neon.h"

int main() {
  print_uint64x1_t(
    vreinterpret_u64_s32(
      set_int32x2_t()));
  return 0;
}
