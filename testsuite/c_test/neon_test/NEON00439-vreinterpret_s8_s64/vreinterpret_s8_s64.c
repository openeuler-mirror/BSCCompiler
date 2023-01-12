#include "neon.h"

int main() {
  print_int8x8_t(
    vreinterpret_s8_s64(
      set_int64x1_t()));
  return 0;
}
