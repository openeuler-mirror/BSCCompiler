#include "neon.h"

int main() {
  print_int16x4_t(
    vget_high_s16(
      set_int16x8_t()));
  return 0;
}
