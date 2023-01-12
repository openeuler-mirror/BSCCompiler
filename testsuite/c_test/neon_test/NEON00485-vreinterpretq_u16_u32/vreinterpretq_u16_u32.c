#include "neon.h"

int main() {
  print_uint16x8_t(
    vreinterpretq_u16_u32(
      set_uint32x4_t()));
  return 0;
}
