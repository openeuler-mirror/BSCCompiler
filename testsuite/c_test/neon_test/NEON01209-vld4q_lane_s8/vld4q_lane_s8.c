#include "neon.h"

int main() {
  print_int8x16x4_t(
    vld4q_lane_s8(
      set_int8_t_ptr(64),
      set_int8x16x4_t(),
      1));
  return 0;
}
