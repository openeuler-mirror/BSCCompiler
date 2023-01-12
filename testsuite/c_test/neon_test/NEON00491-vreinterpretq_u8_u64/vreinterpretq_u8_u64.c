#include "neon.h"

int main() {
  print_uint8x16_t(
    vreinterpretq_u8_u64(
      set_uint64x2_t()));
  return 0;
}
