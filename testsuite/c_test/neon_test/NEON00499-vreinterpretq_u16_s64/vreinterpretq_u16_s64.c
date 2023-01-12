#include "neon.h"

int main() {
  print_uint16x8_t(
    vreinterpretq_u16_s64(
      set_int64x2_t()));
  return 0;
}
