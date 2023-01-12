#include "neon.h"

int main() {
  print_uint64_t(
    vcltd_s64(
      set_int64_t(),
      set_int64_t()));
  return 0;
}
