#include "neon.h"

int main() {
  print_int64x1_t(
    vreinterpret_s64_s32(
      set_int32x2_t()));
  return 0;
}
