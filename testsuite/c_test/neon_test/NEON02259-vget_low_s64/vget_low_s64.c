#include "neon.h"

int main() {
  print_int64x1_t(
    vget_low_s64(
      set_int64x2_t()));
  return 0;
}
