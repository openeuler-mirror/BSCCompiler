#include "neon.h"

int main() {
  print_int32x4_t(
    vuqaddq_s32(
      set_int32x4_t(),
      set_uint32x4_t()));
  return 0;
}
