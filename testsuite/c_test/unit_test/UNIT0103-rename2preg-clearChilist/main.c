#include <stdint.h>
int32_t a;
int32_t *b;
int32_t *c(int64_t, int16_t, uint16_t, uint64_t *);
int32_t d() {
  int32_t **e = &b;
  int32_t ***f = &e;
  int32_t ****g = &f;
  uint32_t h[3];
  int i = 0;
  for (; i < 3; i++)
    ;
  for (a = 0; 0;)
    ;
  if (h) {
    int32_t j[2];
    int64_t k[4][8][5];
    for (i = 0; i < 2; i++)
      j[i] = 8;
    for (; 0;)
      ;
    if (j[1]) {
      int32_t *l;
      int32_t **m = &l;
      uint64_t n;
      for (; a; ++a)
        *g = &m;
      ***g = c(k[3][5][4], 0, 0, &n);
    }
  }
  return 0;
}
int32_t *c(int64_t o, int16_t p, uint16_t q, uint64_t *r) {
  int32_t *s[3];
  for (; 0;)
    ;
  return s;
}
void main() { d(); }