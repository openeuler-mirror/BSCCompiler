#include "neon.h"

int main() {
  print_uint32x2x3_t(
    vld3_u32(
      set_uint32_t_ptr(6)));
  return 0;
}
