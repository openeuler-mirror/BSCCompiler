#include "neon.h"

int main() {
  print_int32x4_t(
    vqdmlal_high_s16(
      set_int32x4_t(),
      set_int16x8_t(),
      set_int16x8_t()));
  return 0;
}
