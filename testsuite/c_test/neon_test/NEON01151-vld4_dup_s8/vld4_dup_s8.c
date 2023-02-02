#include "neon.h"

int main() {
  print_int8x8x4_t(
    vld4_dup_s8(
      set_int8_t_ptr(32)));
  return 0;
}
