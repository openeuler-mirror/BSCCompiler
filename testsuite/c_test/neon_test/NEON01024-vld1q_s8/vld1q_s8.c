#include "neon.h"

int main() {
  print_int8x16_t(
    vld1q_s8(
      set_int8_t_ptr(16)));
  return 0;
}
