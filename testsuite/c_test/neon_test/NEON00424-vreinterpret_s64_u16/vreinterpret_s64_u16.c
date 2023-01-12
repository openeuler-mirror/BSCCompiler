#include "neon.h"

int main() {
  print_int64x1_t(
    vreinterpret_s64_u16(
      set_uint16x4_t()));
  return 0;
}
