#include <stdint.h>
struct {
  int32_t a;
} b;
int64_t c, e;
uint64_t **d;
int32_t *f = &b.a;
int8_t g() {
  int32_t h;
  int16_t k[2][1];
  int i, j;
  i = j = 0;
  k[i][j] = 0;
  uint8_t l = 4;
  for (c = 5; c <= 4;) {
    **d = h;
    if (l)
      break;
  }
  *f = e == e && c;
  return k[0][0];
}
int main() {
  g();
  printf("%d\n", c);
}
