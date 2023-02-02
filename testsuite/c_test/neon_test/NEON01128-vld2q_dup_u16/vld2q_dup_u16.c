#include "neon.h"

int main() {
  print_uint16x8x2_t(
    vld2q_dup_u16(
      set_uint16_t_ptr(16)));
  return 0;
}
