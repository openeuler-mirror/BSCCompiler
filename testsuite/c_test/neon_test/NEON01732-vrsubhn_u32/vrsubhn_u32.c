#include "neon.h"

int main() {
  print_uint16x4_t(
    vrsubhn_u32(
      set_uint32x4_t(),
      set_uint32x4_t()));
  return 0;
}
