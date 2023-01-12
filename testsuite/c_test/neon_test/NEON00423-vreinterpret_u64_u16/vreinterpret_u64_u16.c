#include "neon.h"

int main() {
  print_uint64x1_t(
    vreinterpret_u64_u16(
      set_uint16x4_t()));
  return 0;
}
