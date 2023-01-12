#include "neon.h"

int main() {
  print_int16_t(
    vgetq_lane_s16(
      set_int16x8_t(),
      set_int()));
  return 0;
}
