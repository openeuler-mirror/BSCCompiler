#include "neon.h"

int main() {
  print_int64x2_t(
    vreinterpretq_s64_s16(
      set_int16x8_t()));
  return 0;
}
