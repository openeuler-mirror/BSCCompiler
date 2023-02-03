#include "neon.h"

int main() {
  print_uint32x4_t(
    vabdl_high_u16(
      set_uint16x8_t(),
      set_uint16x8_t()));
  return 0;
}
