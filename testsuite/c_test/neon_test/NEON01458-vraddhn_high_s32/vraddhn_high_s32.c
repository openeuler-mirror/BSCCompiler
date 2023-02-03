#include "neon.h"

int main() {
  print_int16x8_t(
    vraddhn_high_s32(
      set_int16x4_t(),
      set_int32x4_t(),
      set_int32x4_t()));
  return 0;
}
