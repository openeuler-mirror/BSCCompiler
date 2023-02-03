#include "neon.h"

int main() {
  print_int16x4_t(
    vpadal_s8(
      set_int16x4_t(),
      set_int8x8_t()));
  return 0;
}
