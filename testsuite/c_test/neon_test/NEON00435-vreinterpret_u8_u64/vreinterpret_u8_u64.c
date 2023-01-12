#include "neon.h"

int main() {
  print_uint8x8_t(
    vreinterpret_u8_u64(
      set_uint64x1_t()));
  return 0;
}
