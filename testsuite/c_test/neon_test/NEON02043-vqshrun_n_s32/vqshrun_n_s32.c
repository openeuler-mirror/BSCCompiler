#include "neon.h"

int main() {
  print_uint16x4_t(
    vqshrun_n_s32(
      set_int32x4_t(),
      1));
  return 0;
}
