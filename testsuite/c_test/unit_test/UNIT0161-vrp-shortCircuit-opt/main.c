void printf();
short a, g;
signed char b = 5, h;
signed char *c = &b;
unsigned long d = 4073709551613;
unsigned long *e = &d, *i = &d;
int f, j;
static void fn1() {}
signed char fn2() { return 1; }
void fn3() { f = fn4(); }
int fn4(unsigned long p1) {
  p1 = 0;
  for (p1; p1 < 2; p1++)
    ;
  *c = 0 || p1;
  h = fn2(0 < (*i = p1) >= 4);
  g = a - 8;
  for (g; g >= 0; g)
    for (1; 1 < 5; 1)
      e = &p1;
  return 0;
}
int main() {
  fn3();
  for (j; j < 2; j++)
    printf("%d\n", d);
}
