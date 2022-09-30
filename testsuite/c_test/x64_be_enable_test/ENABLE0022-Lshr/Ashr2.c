#include <stdio.h>

void testAshr64bit() {
  long long a = 8589934592;
  long long b = a >> 31;
  printf("%lld %lld\n", a, b);
}

void testLshr32bit() {
  int a = 123456;
  int b = a >> 4;
  printf("%d %d\n", a, b);
}

void testLshr16bit() {
  short a = 12345;
  short b = a >> 4;
  printf("%d %d\n", a, b);
}

int main() {
    testLshr64bit();
    testLshr32bit();
    testLshr16bit();
}
