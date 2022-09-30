#include "stdio.h"

int DivInt(int a, int b) {
  return a/b;
}

int main() {
  int a = 5;
  int b = 3;
  int c = DivInt(a, b);
  printf("%d", c);
  return 0;
}
