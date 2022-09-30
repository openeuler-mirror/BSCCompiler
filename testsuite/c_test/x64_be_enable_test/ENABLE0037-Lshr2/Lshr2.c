#include <stdio.h>
//test 64bit Lshr
void testLshr64bit() {
  //test lshr(64bit)
  unsigned long long a = 8589934592;
  unsigned long long b = a >> 31;
  printf("%llu %llu\n", a, b);
}

void testLshr32bit() {
  unsigned int a = 123456;
  unsigned int b = a >> 4;
  printf("%u %u\n", a, b);
}

void testLshr16bit() {
  unsigned short a = 12345;
  unsigned short b = a >> 4;
  printf("%u %u\n", a, b);
}

int main() {
    testLshr64bit();
    testLshr32bit();
    testLshr16bit();
}
