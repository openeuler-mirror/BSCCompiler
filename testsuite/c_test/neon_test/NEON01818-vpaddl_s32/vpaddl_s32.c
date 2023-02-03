#include "neon.h"

int main() {
  print_int64x1_t(
    vpaddl_s32(
      set_int32x2_t()));
  return 0;
}
