#include "neon.h"

int main() {
  print_int64x1_t(
    vreinterpret_s64_u32(
      set_uint32x2_t()));
  return 0;
}
