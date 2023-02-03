#include "neon.h"

int main() {
  print_int32x2_t(
    vget_low_s32(
      set_int32x4_t()));
  return 0;
}
