#include "neon.h"

int main() {
  print_int32x4_t(
    vmlal_high_n_s16(
      set_int32x4_t(),
      set_int16x8_t(),
      set_int16_t()));
  return 0;
}
