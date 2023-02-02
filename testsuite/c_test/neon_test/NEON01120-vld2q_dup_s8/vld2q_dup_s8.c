#include "neon.h"

int main() {
  print_int8x16x2_t(
    vld2q_dup_s8(
      set_int8_t_ptr(32)));
  return 0;
}
