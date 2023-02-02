#include "neon.h"

int main() {
  print_uint32x4x2_t(
    vld2q_u32(
      set_uint32_t_ptr(8)));
  return 0;
}
