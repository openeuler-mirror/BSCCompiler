#include "neon.h"

int main() {
  print_uint32x2x2_t(
    vld2_u32(
      set_uint32_t_ptr(4)));
  return 0;
}
