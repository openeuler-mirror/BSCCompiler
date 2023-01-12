#include "neon.h"

int main() {
  print_uint16x4_t(
    vqmovn_u32(
      set_uint32x4_t()));
  return 0;
}
