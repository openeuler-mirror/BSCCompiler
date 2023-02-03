#include "neon.h"

int main() {
  print_int64x2_t(
    vpadalq_s32(
      set_int64x2_t(),
      set_int32x4_t()));
  return 0;
}
