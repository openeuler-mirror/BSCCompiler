#include "neon.h"

int main() {
  uint64_t ptr[8] = { 0 };
  vst4q_u64(
      ptr,
      set_uint64x2x4_t());
  print_uint64_t_ptr(ptr, 8);
  return 0;
}
