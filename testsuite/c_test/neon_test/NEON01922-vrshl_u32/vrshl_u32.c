#include "neon.h"

int main() {
  print_uint32x2_t(
    vrshl_u32(
      set_uint32x2_t(),
      set_int32x2_t()));
  return 0;
}
