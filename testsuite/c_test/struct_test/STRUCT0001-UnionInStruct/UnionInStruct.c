#include <stdlib.h>
#include <stdio.h>

struct S0 {
  int a:9;
  int : 0;
  int b:25;
};

union U {
  int a;
  char *b;
  struct S0 s0;
};

struct S1 {
  union U u;
  struct S0 s0;
  int c : 18;
  int d : 25;
};

struct S1 g_12 = {
  .u.s0 = {6, 999},
  .s0 = {9, 666},
  .c = 888,
  .d = 11111111
};

int main() {
  printf("g_12.u.s0.b = %d, g_12.s0.b = %d, g_12.c = %d, g_12.d = %d\n", g_12.u.s0.b, g_12.s0.b, g_12.c, g_12.d);
  return 0;
}
