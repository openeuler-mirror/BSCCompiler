#include "neon.h"

int main() {
  print_int32x4_t(
    veorq_s32(
      set_int32x4_t(),
      set_int32x4_t()));
  return 0;
}
