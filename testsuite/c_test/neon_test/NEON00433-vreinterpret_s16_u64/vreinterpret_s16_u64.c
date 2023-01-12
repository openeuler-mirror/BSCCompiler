#include "neon.h"

int main() {
  print_int16x4_t(
    vreinterpret_s16_u64(
      set_uint64x1_t()));
  return 0;
}
