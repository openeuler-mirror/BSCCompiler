#include "neon.h"

int main() {
  print_int32_t(
    vqdmulhs_s32(
      set_int32_t(),
      set_int32_t()));
  return 0;
}
