#include "neon.h"

int main() {
  print_int64x2_t(
    vreinterpretq_s64_s8(
      set_int8x16_t()));
  return 0;
}
