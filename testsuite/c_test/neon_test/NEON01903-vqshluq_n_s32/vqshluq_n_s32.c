#include "neon.h"

int main() {
  print_uint32x4_t(
    vqshluq_n_s32(
      set_int32x4_t(),
      1));
  return 0;
}
