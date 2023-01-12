#include "neon.h"

int main() {
  print_int32x4_t(
    vreinterpretq_s32_u16(
      set_uint16x8_t()));
  return 0;
}
