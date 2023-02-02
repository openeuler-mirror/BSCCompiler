#include "neon.h"

int main() {
  print_int8x8_t(
    vld1_dup_s8(
      set_int8_t_ptr(8)));
  return 0;
}
