#include "neon.h"

int main() {
  print_uint32x4x3_t(
    vld3q_u32(
      set_uint32_t_ptr(12)));
  return 0;
}
