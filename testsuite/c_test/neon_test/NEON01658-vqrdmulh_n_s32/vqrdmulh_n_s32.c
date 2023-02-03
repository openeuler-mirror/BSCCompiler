#include "neon.h"

int main() {
  print_int32x2_t(
    vqrdmulh_n_s32(
      set_int32x2_t(),
      set_int32_t()));
  return 0;
}
