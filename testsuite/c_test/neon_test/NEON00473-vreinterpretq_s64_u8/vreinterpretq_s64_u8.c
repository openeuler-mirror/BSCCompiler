#include "neon.h"

int main() {
  print_int64x2_t(
    vreinterpretq_s64_u8(
      set_uint8x16_t()));
  return 0;
}
