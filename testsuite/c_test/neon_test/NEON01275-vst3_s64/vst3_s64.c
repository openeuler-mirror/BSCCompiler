#include "neon.h"

int main() {
  int64_t ptr[3] = { 0 };
  vst3_s64(
      ptr,
      set_int64x1x3_t());
  print_int64_t_ptr(ptr, 3);
  return 0;
}
