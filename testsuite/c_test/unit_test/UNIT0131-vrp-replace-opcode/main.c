#include <stdio.h>
long long a;
int c = 2;
char d = 6;
void e(long long *p1, int i) { *p1 = i; }
void fn2();
int main() {
  fn2(7);
  e(&a, d);
  printf("%llu\n", a);
}
#define bi(f, g) f < g ? f : 0
void fn2(p1) {
  for (unsigned b = bi((c ? -1 : 4054300999ULL), 4); b < p1; b += 4)
    d = 0;
}
