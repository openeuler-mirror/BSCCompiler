#include <stdio.h>
extern void abort();
__attribute__((__noinline__))
void test(unsigned int a, unsigned int b) {
  if (a == 0 || a < b) {
    abort();
  }
}

int main() {
  test(0, 0);
}
