#include <stdio.h>

signed char b = -4;
int main() {
  long long a = 0;
  a = b ?: 0UL;
  printf("%llu\n", a);
  return 0;
}
