#include "neon.h"

int main() {
  print_int32x4_t(
    vbslq_s32(
      set_uint32x4_t(),
      set_int32x4_t(),
      set_int32x4_t()));
  return 0;
}
