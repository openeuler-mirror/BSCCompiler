#include "neon.h"

int main() {
  print_uint32x4_t(
    vcombine_u32(
      set_uint32x2_t(),
      set_uint32x2_t()));
  return 0;
}
