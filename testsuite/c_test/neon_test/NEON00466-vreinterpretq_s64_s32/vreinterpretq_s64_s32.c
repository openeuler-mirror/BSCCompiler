#include "neon.h"

int main() {
  print_int64x2_t(
    vreinterpretq_s64_s32(
      set_int32x4_t()));
  return 0;
}
