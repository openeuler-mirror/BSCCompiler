#include "neon.h"

int main() {
  print_int8x16_t(
    vreinterpretq_s8_u64(
      set_uint64x2_t()));
  return 0;
}
