#include "neon.h"

int main() {
  print_int64x1_t(
    vget_high_s64(
      set_int64x2_t()));
  return 0;
}
