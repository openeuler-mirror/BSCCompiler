#include "neon.h"

int main() {
  print_int32x2_t(
    vreinterpret_s32_s64(
      set_int64x1_t()));
  return 0;
}
