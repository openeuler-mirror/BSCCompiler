#include <stdio.h>
int a;
long b;
long *c = &b;
short d;
unsigned e;
short *f = &d;
int main() {
  int *g = &a;
  *g = *c = 18446744073709551613;
  *f = (e = a) && *g;
  printf("%d\n", d);
}
