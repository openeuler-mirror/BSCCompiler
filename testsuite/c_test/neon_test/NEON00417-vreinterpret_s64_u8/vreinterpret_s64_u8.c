#include "neon.h"

int main() {
  print_int64x1_t(
    vreinterpret_s64_u8(
      set_uint8x8_t()));
  return 0;
}
