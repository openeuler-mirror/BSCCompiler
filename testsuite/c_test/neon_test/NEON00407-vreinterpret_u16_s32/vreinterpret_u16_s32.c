#include "neon.h"

int main() {
  print_uint16x4_t(
    vreinterpret_u16_s32(
      set_int32x2_t()));
  return 0;
}
