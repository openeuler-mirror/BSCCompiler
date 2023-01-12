#include "neon.h"

int main() {
  print_int16x4_t(
    vreinterpret_s16_u16(
      set_uint16x4_t()));
  return 0;
}
