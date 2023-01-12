#include "neon.h"

int main() {
  print_int32x4_t(
    vreinterpretq_s32_u32(
      set_uint32x4_t()));
  return 0;
}
