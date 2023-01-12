#include "neon.h"

int main() {
  print_uint64x2_t(
    vreinterpretq_u64_u8(
      set_uint8x16_t()));
  return 0;
}
