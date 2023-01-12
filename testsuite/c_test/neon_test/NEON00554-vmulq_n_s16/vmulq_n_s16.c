#include "neon.h"

int main() {
  print_int16x8_t(
    vmulq_n_s16(
      set_int16x8_t(),
      set_int16_t()));
  return 0;
}
