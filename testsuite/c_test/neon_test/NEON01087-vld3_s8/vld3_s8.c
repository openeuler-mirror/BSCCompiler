#include "neon.h"

int main() {
  print_int8x8x3_t(
    vld3_s8(
      set_int8_t_ptr(24)));
  return 0;
}
