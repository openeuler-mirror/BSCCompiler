#include "neon.h"

int main() {
  print_int64x1_t(
    vreinterpret_s64_s16(
      set_int16x4_t()));
  return 0;
}
