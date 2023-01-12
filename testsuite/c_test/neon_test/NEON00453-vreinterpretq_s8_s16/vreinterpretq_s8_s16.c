#include "neon.h"

int main() {
  print_int8x16_t(
    vreinterpretq_s8_s16(
      set_int16x8_t()));
  return 0;
}
