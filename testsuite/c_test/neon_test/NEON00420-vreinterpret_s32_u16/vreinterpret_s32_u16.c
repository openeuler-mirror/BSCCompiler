#include "neon.h"

int main() {
  print_int32x2_t(
    vreinterpret_s32_u16(
      set_uint16x4_t()));
  return 0;
}
