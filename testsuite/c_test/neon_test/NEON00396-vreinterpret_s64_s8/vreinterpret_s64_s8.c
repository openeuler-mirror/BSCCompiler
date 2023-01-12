#include "neon.h"

int main() {
  print_int64x1_t(
    vreinterpret_s64_s8(
      set_int8x8_t()));
  return 0;
}
