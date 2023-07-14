#include <stdio.h>

long long a;
char c[2];
unsigned d[6];
void b(long long *e, int p2) { *e ^= p2; }
int main() {
  for (size_t f = 0; f < 2; ++f)
    c[f] = 238;
  for (_Bool g = 0; g < (_Bool)3; g = 7)
    d[2] = (long)(signed char)c[g];
  for (size_t f = 0; f < 6; ++f)
    b(&a, d[f]);
  printf("%llu\n", a);
}

