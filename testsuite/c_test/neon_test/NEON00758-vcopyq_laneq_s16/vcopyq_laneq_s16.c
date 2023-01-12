#include "neon.h"

int main() {
  print_int16x8_t(
    vcopyq_laneq_s16(
      set_int16x8_t(),
      set_int(),
      set_int16x8_t(),
      set_int()));
  return 0;
}
