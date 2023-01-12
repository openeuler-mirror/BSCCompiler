#include "neon.h"

int main() {
  print_uint16x4_t(
    vreinterpret_u16_s64(
      set_int64x1_t()));
  return 0;
}
