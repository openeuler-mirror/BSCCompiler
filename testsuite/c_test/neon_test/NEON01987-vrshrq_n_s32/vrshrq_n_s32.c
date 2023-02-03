#include "neon.h"

int main() {
  print_int32x4_t(
    vrshrq_n_s32(
      set_int32x4_t(),
      1));
  return 0;
}
