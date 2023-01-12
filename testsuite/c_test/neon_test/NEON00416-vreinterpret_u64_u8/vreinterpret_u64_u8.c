#include "neon.h"

int main() {
  print_uint64x1_t(
    vreinterpret_u64_u8(
      set_uint8x8_t()));
  return 0;
}
