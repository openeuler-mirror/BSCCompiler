#include "neon.h"

int main() {
  print_uint16x4_t(
    vreinterpret_u16_u64(
      set_uint64x1_t()));
  return 0;
}
