#include "neon.h"

int main() {
  print_int16x8_t(
    vuzp1q_s16(
      set_int16x8_t(),
      set_int16x8_t()));
  return 0;
}
