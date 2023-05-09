#include "csmith.h"

struct a {
  uint16_t c;
  uint64_t d;
  int64_t e;
  int8_t f;
  uint64_t g;
  uint64_t h;
};

struct {
  uint32_t b;
  struct a i;
} k[1][9][6];

int32_t j[][1];

int32_t m(l) {
  int32_t *n = &j[2][6];
  *n = safe_mul_func_int8_t_s_s(l, k[1][4][2].b);
  return 0;
}

int main() {
  return 0;
}
