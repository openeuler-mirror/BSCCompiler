#include "neon.h"

int main() {
  print_uint16x8_t(
    vqmovun_high_s32(
      set_uint16x4_t(),
      set_int32x4_t()));
  return 0;
}
