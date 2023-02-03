#include "neon.h"

int main() {
  print_int32x4_t(
    vmlsq_n_s32(
      set_int32x4_t(),
      set_int32x4_t(),
      set_int32_t()));
  return 0;
}
