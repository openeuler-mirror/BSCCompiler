#include "neon.h"

int main() {
  print_int32x4_t(
    vmovl_high_s16(
      set_int16x8_t()));
  return 0;
}
