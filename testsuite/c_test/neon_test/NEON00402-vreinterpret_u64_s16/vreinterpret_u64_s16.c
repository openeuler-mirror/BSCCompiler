#include "neon.h"

int main() {
  print_uint64x1_t(
    vreinterpret_u64_s16(
      set_int16x4_t()));
  return 0;
}
