#include "neon.h"

int main() {
  print_uint64x2_t(
    vreinterpretq_u64_s16(
      set_int16x8_t()));
  return 0;
}
