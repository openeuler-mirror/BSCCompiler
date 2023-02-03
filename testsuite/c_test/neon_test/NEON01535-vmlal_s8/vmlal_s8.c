#include "neon.h"

int main() {
  print_int16x8_t(
    vmlal_s8(
      set_int16x8_t(),
      set_int8x8_t(),
      set_int8x8_t()));
  return 0;
}
