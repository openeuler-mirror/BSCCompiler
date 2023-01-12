#include "neon.h"

int main() {
  print_uint64x2_t(
    vreinterpretq_u64_u16(
      set_uint16x8_t()));
  return 0;
}
