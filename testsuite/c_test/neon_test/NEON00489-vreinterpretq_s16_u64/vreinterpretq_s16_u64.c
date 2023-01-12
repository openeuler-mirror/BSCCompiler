#include "neon.h"

int main() {
  print_int16x8_t(
    vreinterpretq_s16_u64(
      set_uint64x2_t()));
  return 0;
}
