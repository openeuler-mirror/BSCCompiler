#include "neon.h"

int main() {
  print_int8_t(
    vdupb_laneq_s8(
      set_int8x16_t(),
      set_int()));
  return 0;
}
