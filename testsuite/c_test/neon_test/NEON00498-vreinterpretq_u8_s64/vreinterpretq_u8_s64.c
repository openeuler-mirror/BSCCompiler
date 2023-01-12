#include "neon.h"

int main() {
  print_uint8x16_t(
    vreinterpretq_u8_s64(
      set_int64x2_t()));
  return 0;
}
