#include "stdio.h"

#define c(a, b)                                                                \
  {                                                                            \
    __typeof__(a) d = a;                                                       \
    __typeof__(b) n = b;                                                       \
    d < n;                                                                     \
  }

extern short var_18;

void test(int e, int f, _Bool g, char h, unsigned i, int o, int j, short k, unsigned long l, int m) {
  var_18 = (c(m ? l : i + 2, k ? j : 0));
  printf("var_18 = %d\n", var_18);
}