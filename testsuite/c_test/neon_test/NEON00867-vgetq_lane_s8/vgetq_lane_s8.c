#include "neon.h"

int main() {
  print_int8_t(
    vgetq_lane_s8(
      set_int8x16_t(),
      1));
  return 0;
}
