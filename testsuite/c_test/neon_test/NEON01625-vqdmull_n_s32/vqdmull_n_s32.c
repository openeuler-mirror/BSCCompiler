#include "neon.h"

int main() {
  print_int64x2_t(
    vqdmull_n_s32(
      set_int32x2_t(),
      set_int32_t()));
  return 0;
}
