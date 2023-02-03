#include "neon.h"

int main() {
  print_int64x2_t(
    vmlsl_n_s32(
      set_int64x2_t(),
      set_int32x2_t(),
      set_int32_t()));
  return 0;
}
