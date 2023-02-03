#include "neon.h"

int main() {
  print_int64x2_t(
    vsubl_s32(
      set_int32x2_t(),
      set_int32x2_t()));
  return 0;
}
