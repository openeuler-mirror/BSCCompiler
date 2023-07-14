unsigned char a, e, f;
struct U2 {
  int f1;
} b, j, l;
int c[32];
int *d;
volatile long g;
short h = 6;
int i, n = 0, r, s;
unsigned short m, p = 9;
int *o = &n;
void(fn1)();
static unsigned char(fn2)(unsigned char p1) {
  return a + p1;
}
unsigned short fn3() {
  for (i; i < 3; i++) g = 0;
  g++;
  for (1; 1 < 1; 1)
    ;
  return 9;
}
struct U2 fn4() {
  int k[] = {1, 3, 1, 0, 4, 4, 0, 1, 3, 3, 4, 1, 0, 1, 4, 3};
  return j;
}
unsigned int fn5() {
  int q = 4;
  for (q;; q) {
    l = fn4();
    m = fn3();
    p = 0 == ((d = o = &q) != &c);
    for (f; f <= 4; f++) h = 0;
    for (0; e <= 5; e++) c[q * 4 + 0] = fn2(fn1 || 0);
    return 1;
  }
}
int main() {
  fn5();
  printf("%d\n", c[0]);
}