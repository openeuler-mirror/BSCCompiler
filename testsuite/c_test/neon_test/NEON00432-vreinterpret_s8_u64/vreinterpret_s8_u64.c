#include "neon.h"

int main() {
  print_int8x8_t(
    vreinterpret_s8_u64(
      set_uint64x1_t()));
  return 0;
}
