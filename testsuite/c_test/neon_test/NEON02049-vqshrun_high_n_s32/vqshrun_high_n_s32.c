#include "neon.h"

int main() {
  print_uint16x8_t(
    vqshrun_high_n_s32(
      set_uint16x4_t(),
      set_int32x4_t(),
      1));
  return 0;
}
