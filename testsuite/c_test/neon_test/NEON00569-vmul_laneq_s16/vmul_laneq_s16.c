#include "neon.h"

int main() {
  print_int16x4_t(
    vmul_laneq_s16(
      set_int16x4_t(),
      set_int16x8_t(),
      set_int()));
  return 0;
}
