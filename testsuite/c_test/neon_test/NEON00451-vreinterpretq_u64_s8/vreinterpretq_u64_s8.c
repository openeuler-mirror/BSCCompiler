#include "neon.h"

int main() {
  print_uint64x2_t(
    vreinterpretq_u64_s8(
      set_int8x16_t()));
  return 0;
}
