#include "neon.h"

int main() {
  print_int16x4_t(
    vdup_laneq_s16(
      set_int16x8_t(),
      set_int()));
  return 0;
}
