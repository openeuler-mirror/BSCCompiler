#include "neon.h"

int main() {
  print_uint32x4_t(
    vcleq_s32(
      set_int32x4_t(),
      set_int32x4_t()));
  return 0;
}
