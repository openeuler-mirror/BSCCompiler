#include "neon.h"

int main() {
  print_int16x4_t(
    vreinterpret_s16_s64(
      set_int64x1_t()));
  return 0;
}
