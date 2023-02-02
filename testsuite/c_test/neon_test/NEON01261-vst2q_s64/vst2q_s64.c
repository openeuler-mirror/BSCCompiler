#include "neon.h"

int main() {
  int64_t ptr[4] = { 0 };
  vst2q_s64(
      ptr,
      set_int64x2x2_t());
  print_int64_t_ptr(ptr, 4);
  return 0;
}
