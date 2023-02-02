#include "neon.h"

int main() {
  print_int16_t(
    vduph_laneq_s16(
      set_int16x8_t(),
      1));
  return 0;
}
