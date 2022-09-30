#include <stdio.h>

void testAshr64bit() {
  long long a = 8589934592;
  long long b = a >> 31;
  printf("%lld %lld\n", a, b);
}

void testAshr32bit() {
  int a = 123456;
  int b = a >> 4;
  printf("%d %d\n", a, b);
}

void testAshr16bit() {
  short a = 12345;
  short b = a >> 4;
  printf("%d %d\n", a, b);
}

int main() {
    testAshr64bit();
    testAshr32bit();
    testAshr16bit();
}
