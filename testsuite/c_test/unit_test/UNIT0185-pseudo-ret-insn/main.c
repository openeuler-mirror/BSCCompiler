#include <stdio.h>

char a, b = 6;

struct c {
  short d;
  int e;
  short f;
} g, h, *k = &h;

volatile signed char *i = &b;
volatile signed char **j = &i;

void(l)(m) { 
  m == 0 || a &&m == 1;
}

struct c n() {
  l(**j);
  return g;
}

int main() {
  *k = n();
  printf("%d\n", h.f);
}
