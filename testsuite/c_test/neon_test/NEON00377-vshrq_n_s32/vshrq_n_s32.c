#include "neon.h"

int main() {
  print_int32x4_t(
    vshrq_n_s32(
      set_int32x4_t(),
      set_int_1()));
  return 0;
}
