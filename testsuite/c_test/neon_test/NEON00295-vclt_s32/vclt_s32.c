#include "neon.h"

int main() {
  print_uint32x2_t(
    vclt_s32(
      set_int32x2_t(),
      set_int32x2_t()));
  return 0;
}
