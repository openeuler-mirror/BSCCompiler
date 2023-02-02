#include "neon.h"

int main() {
  print_int16x4x4_t(
    vld4_dup_s16(
      set_int16_t_ptr(16)));
  return 0;
}
