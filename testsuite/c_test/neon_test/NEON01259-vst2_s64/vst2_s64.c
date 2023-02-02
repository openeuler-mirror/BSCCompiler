#include "neon.h"

int main() {
  int64_t ptr[2] = { 0 };
  vst2_s64(
      ptr,
      set_int64x1x2_t());
  print_int64_t_ptr(ptr, 2);
  return 0;
}
