#include "neon.h"

int main() {
  print_int32x2_t(
    vshr_n_s32(
      set_int32x2_t(),
      set_int_1()));
  return 0;
}
