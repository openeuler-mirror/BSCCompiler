#include <stdio.h>
long long a;
int b;
int c[];
short d[][13][11][18];
long long e[][13][11][18];
char f;
void g(long long *p1, int i) { *p1 = i; }
void fn2(int, unsigned char, short[][13][11][18], long long[][13][11][18]);
int main() {
  fn2(601, 2, d, e);
  g(&a, b);
  printf("%llu\n", a);
}
void fn2(int p1, unsigned char i, short j[][13][11][18],
         long long m[][13][11][18]) {
  for (int k = 0; k < 4; k = p1)
    if (c)
      for (int l = 0; l < 40933431; l++)
        b = f - i ? 0 : 90807144187420;
}

