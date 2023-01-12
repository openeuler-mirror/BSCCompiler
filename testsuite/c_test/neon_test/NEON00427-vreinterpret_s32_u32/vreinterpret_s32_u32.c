#include "neon.h"

int main() {
  print_int32x2_t(
    vreinterpret_s32_u32(
      set_uint32x2_t()));
  return 0;
}
