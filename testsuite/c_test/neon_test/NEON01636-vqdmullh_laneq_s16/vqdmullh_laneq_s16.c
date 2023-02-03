#include "neon.h"

int main() {
  print_int32_t(
    vqdmullh_laneq_s16(
      set_int16_t(),
      set_int16x8_t(),
      1));
  return 0;
}
