#include "neon.h"

int main() {
  print_uint16x8_t(
    vreinterpretq_u16_s32(
      set_int32x4_t()));
  return 0;
}
