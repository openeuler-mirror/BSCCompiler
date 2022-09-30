int callconv(int a, int b, int c, int d, int e, int f) {
  return a + b + c + d + e + f;
}
int main() {
  int a = 0;
  int b = 1;
  int c = 2;
  int d = 3;
  int e = 4;
  int f = 5;
  int h = callconv(a, b, c, d, e, f);
  printf("%d", h);
  return 0;
}
