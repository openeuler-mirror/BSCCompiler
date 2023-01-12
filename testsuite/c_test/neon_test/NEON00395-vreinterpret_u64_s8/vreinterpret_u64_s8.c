#include "neon.h"

int main() {
  print_uint64x1_t(
    vreinterpret_u64_s8(
      set_int8x8_t()));
  return 0;
}
