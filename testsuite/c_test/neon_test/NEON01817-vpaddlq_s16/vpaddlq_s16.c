#include "neon.h"

int main() {
  print_int32x4_t(
    vpaddlq_s16(
      set_int16x8_t()));
  return 0;
}
