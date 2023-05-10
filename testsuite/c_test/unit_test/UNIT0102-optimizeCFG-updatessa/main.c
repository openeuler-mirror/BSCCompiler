#include <stdio.h>
long long a;
short b, c;
int d = 9;
long e;
char f;
void g(long long *p1, int i) { *p1 = i; }
void fn2(short);
int main() {
  fn2(80);
  g(&a, d);
  printf("%llu\n", a);
}
void fn2(short p1) {
  if (p1) {
    if (f)
      b = 0;
    if (c ? p1 : 6)
      d = 0;
    (_Bool) f || e;
  }
}
