#include "neon.h"

int main() {
  print_int64x2_t(
    vreinterpretq_s64_u32(
      set_uint32x4_t()));
  return 0;
}
