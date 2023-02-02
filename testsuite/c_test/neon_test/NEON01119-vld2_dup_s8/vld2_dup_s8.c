#include "neon.h"

int main() {
  print_int8x8x2_t(
    vld2_dup_s8(
      set_int8_t_ptr(16)));
  return 0;
}
