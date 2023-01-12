#include "neon.h"

int main() {
  print_int32x2_t(
    vreinterpret_s32_s8(
      set_int8x8_t()));
  return 0;
}
