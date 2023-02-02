#include "neon.h"

int main() {
  int64_t ptr[1] = { 0 };
  vst1_s64(
      ptr,
      set_int64x1_t());
  print_int64_t_ptr(ptr, 1);
  return 0;
}
