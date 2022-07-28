#include <stdio.h>

struct Agg {
  int i;
  int j;
} gAgg;

void foo() {
  struct Agg {
    float m;
    float n;
  } tstAgg;

  tstAgg.m = 1.0;
  tstAgg.n = 2.0;

  gAgg.i = 1.0;
  gAgg.j = 2.0;

  printf("%f %f %d %d\n", tstAgg.m, tstAgg.n, gAgg.i, gAgg.j);
}

int main() {
  foo();
  return 0;
}
