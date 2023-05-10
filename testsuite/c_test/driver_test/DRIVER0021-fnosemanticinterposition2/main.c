#include <stdio.h>

extern int foo();

int main() {
  int x = foo();
  printf("%d", x);
  return 0;
}