#include "neon.h"

int main() {
  print_uint32x4_t(
    vshlq_u32(
      set_uint32x4_t(),
      set_int32x4_t()));
  return 0;
}
