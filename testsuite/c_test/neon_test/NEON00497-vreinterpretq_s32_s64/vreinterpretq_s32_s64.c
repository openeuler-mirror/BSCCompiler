#include "neon.h"

int main() {
  print_int32x4_t(
    vreinterpretq_s32_s64(
      set_int64x2_t()));
  return 0;
}
