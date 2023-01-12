#include "neon.h"

int main() {
  print_int8x8_t(
    vreinterpret_s8_s32(
      set_int32x2_t()));
  return 0;
}
