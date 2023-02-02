#include "neon.h"

int main() {
  int64_t ptr[4] = { 0 };
  vst4_s64(
      ptr,
      set_int64x1x4_t());
  print_int64_t_ptr(ptr, 4);
  return 0;
}
