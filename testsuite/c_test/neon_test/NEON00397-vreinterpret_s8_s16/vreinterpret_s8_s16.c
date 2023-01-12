#include "neon.h"

int main() {
  print_int8x8_t(
    vreinterpret_s8_s16(
      set_int16x4_t()));
  return 0;
}
