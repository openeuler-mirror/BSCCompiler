#include <stdio.h>

struct Header {
  unsigned short a;
  unsigned short b;
} header;

int test1() {
  // CHECK-NOT:  rev16
  // CHECK:  and{{.*}}#4280287007
  unsigned short n = ((header.a & (0xFF00)) >> 8) | ((header.a & (0x00FF)) << 8);
  if (n & 0x1FFF) {
    printf("case1 right\n");
  } else {
    printf("case1 wrong\n");
  }

  return 0;
}

int test2(unsigned int n) {
  // CHECK-NOT:  rev
  // CHECK:  and{{.*}}#4282384191
  // CHECK:  eor{{.*}}#4282384191
  // CHECK:  orr{{.*}}#4282384191
  n = (((n & 0x000000FF) << 24) | ((n & 0x0000FF00) << 8) |
       ((n & 0x00FF0000) >> 8)  | ((n & 0xFF000000) >> 24));
  n &= 0x3FFF3FFF;
  n ^= 0x3FFF3FFF;
  n |= 0x3FFF3FFF;
  if (n) {
    printf("case2 right\n");
  } else {
    printf("case2 wrong\n");
  }

  return 0;
}

int test3(unsigned long long n) {
  // CHECK-NOT:  rev
  // CHECK:  and{{.*}}#6148914691236517205
  n = (((n & 0x00000000000000FF) << 56) | ((n & 0x000000000000FF00) << 40) |
       ((n & 0x0000000000FF0000) << 24) | ((n & 0x00000000FF000000) << 8)  |
       ((n & 0x000000FF00000000) >> 8)  | ((n & 0x0000FF0000000000) >> 24) |
       ((n & 0x00FF000000000000) >> 40) | ((n & 0xFF00000000000000) >> 56));
  n &= 0x5555555555555555ULL; 
  if (n) {
    printf("case3 right\n");
  } else {
    printf("case3 wrong\n");
  }

  return 0;
}

int test4() {
  // CHECK-NOT:  rev16
  // CHECK:  cmp{{.*}}#8
  // CHECK:  beq{{.*}}
  if (((((header.b & (0xFF00)) >> 8) | ((header.b & (0x00FF)) << 8))) != 0x0800) {
    printf("case4 right\n");
  } else {
    printf("case4 wrong\n");
  }

  return 0;
}

int main() {
  header.a = 0x00FF;
  header.b = 0xFF00;
  test1();
  test2(0x00FF00FF);
  test3(0x00FF00FF);
  test4();
}
