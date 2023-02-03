#include "neon.h"

int main() {
  print_int32x4_t(
    vabdl_s16(
      set_int16x4_t(),
      set_int16x4_t()));
  return 0;
}
