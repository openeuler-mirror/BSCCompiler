#include "neon.h"

int main() {
  print_uint8x8_t(
    vreinterpret_u8_s64(
      set_int64x1_t()));
  return 0;
}
