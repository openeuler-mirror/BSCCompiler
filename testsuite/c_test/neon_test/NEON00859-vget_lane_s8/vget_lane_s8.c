#include "neon.h"

int main() {
  print_int8_t(
    vget_lane_s8(
      set_int8x8_t(),
      set_int()));
  return 0;
}
