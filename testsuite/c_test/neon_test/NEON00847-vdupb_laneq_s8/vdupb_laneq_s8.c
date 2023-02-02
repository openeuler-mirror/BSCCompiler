#include "neon.h"

int main() {
  print_int8_t(
    vdupb_laneq_s8(
      set_int8x16_t(),
      1));
  return 0;
}
