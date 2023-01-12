#include "neon.h"

int main() {
  print_int8x16_t(
    vreinterpretq_s8_s64(
      set_int64x2_t()));
  return 0;
}
