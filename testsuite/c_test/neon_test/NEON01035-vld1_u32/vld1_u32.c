#include "neon.h"

int main() {
  print_uint32x2_t(
    vld1_u32(
      set_uint32_t_ptr(2)));
  return 0;
}
