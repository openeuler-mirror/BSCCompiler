#include <stdio.h>

int destVar = -1;
void test01() {
  int srcVar = 4;
  asm volatile ("mov %w0, %w1"
                : "=r"(destVar)
                : "r"(srcVar));
  printf("destVar = %d\n", destVar);
}

typedef struct {
  int a;
  int b;
} Foo;
Foo destFoo = {-1,-1};

void test02() {
  int srcVar = 3;
  asm volatile ("mov %w0, %w1"
                : "=r"(destFoo.b)
                : "r"(srcVar));
  printf("destFoo.a = %d\n", destFoo.a);
  printf("destFoo.b = %d\n", destFoo.b);
}

int main() {
  test01();
  test02();
  return 0;
}
