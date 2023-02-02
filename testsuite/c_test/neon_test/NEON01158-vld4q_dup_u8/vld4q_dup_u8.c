#include "neon.h"

int main() {
  print_uint8x16x4_t(
    vld4q_dup_u8(
      set_uint8_t_ptr(64)));
  return 0;
}
