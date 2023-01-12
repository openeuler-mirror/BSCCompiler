#include "neon.h"

int main() {
  print_int16x8_t(
    vmaxq_s16(
      set_int16x8_t(),
      set_int16x8_t()));
  return 0;
}
