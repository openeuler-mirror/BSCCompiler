#include "neon.h"

int main() {
  print_uint8x8x3_t(
    vld3_u8(
      set_uint8_t_ptr(24)));
  return 0;
}
