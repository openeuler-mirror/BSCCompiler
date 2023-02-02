#include "neon.h"

int main() {
  print_uint8x16x2_t(
    vld2q_u8(
      set_uint8_t_ptr(32)));
  return 0;
}
