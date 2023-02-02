#include "neon.h"

int main() {
  print_uint8x16_t(
    vld1q_dup_u8(
      set_uint8_t_ptr(16)));
  return 0;
}
