#include "neon.h"

int main() {
  print_int8x16x4_t(
    vld4q_s8(
      set_int8_t_ptr(64)));
  return 0;
}
