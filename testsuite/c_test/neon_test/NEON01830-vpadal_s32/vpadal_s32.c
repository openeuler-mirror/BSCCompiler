#include "neon.h"

int main() {
  print_int64x1_t(
    vpadal_s32(
      set_int64x1_t(),
      set_int32x2_t()));
  return 0;
}
