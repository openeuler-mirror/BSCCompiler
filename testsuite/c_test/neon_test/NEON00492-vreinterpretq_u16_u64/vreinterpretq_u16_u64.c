#include "neon.h"

int main() {
  print_uint16x8_t(
    vreinterpretq_u16_u64(
      set_uint64x2_t()));
  return 0;
}
