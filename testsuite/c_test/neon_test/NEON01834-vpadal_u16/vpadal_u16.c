#include "neon.h"

int main() {
  print_uint32x2_t(
    vpadal_u16(
      set_uint32x2_t(),
      set_uint16x4_t()));
  return 0;
}
