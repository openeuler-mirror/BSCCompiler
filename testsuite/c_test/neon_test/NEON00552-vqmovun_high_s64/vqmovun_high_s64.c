#include "neon.h"

int main() {
  print_uint32x4_t(
    vqmovun_high_s64(
      set_uint32x2_t(),
      set_int64x2_t()));
  return 0;
}
