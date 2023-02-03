#include "neon.h"

int main() {
  print_uint32x2_t(
    vqshlu_n_s32(
      set_int32x2_t(),
      1));
  return 0;
}
