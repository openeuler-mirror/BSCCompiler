#include <stdio.h>

typedef struct {
  long a;
  unsigned long b;
} S0;

S0 c[1] = {0, 0};

signed char b = -4;
int main() {
  long long a = 0;
  a = b ?: c[0].b;
  printf("%llu\n", a);
  return 0;
}
