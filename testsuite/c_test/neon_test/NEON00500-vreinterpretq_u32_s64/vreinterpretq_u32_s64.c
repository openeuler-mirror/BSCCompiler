#include "neon.h"

int main() {
  print_uint32x4_t(
    vreinterpretq_u32_s64(
      set_int64x2_t()));
  return 0;
}
