#include "neon.h"

int main() {
  print_int64x2_t(
    vreinterpretq_s64_u16(
      set_uint16x8_t()));
  return 0;
}
