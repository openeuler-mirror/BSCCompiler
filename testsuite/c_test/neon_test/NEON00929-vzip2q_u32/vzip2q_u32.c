#include "neon.h"

int main() {
  print_uint32x4_t(
    vzip2q_u32(
      set_uint32x4_t(),
      set_uint32x4_t()));
  return 0;
}
