#include "neon.h"

int main() {
  print_int32x2_t(
    vzip2_s32(
      set_int32x2_t(),
      set_int32x2_t()));
  return 0;
}
