#include "neon.h"

int main() {
  print_int16x8_t(
    vreinterpretq_s16_s64(
      set_int64x2_t()));
  return 0;
}
