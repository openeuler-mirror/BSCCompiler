#include <stdio.h>
extern void abort();
__attribute__((__noinline__))
void test(unsigned int a, unsigned int b) {
  if (a == 0 || a < b) {
    abort();
  }
}

int main() {
  unsigned int a;
  if (rand()) {
    a = 1;
  } else {
    a = 2;
  }
  test(a, 0);
}
