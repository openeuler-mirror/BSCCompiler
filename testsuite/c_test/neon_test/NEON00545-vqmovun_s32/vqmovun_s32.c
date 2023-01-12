#include "neon.h"

int main() {
  print_uint16x4_t(
    vqmovun_s32(
      set_int32x4_t()));
  return 0;
}
