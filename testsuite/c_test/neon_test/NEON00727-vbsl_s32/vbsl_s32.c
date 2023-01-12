#include "neon.h"

int main() {
  print_int32x2_t(
    vbsl_s32(
      set_uint32x2_t(),
      set_int32x2_t(),
      set_int32x2_t()));
  return 0;
}
