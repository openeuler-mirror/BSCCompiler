#include "neon.h"

int main() {
  print_uint32x2_t(
    vget_low_u32(
      set_uint32x4_t()));
  return 0;
}
