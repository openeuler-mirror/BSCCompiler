int main() {
  int a = 1;
  int b = a << 4;
  int c = a << b;
  printf("%d,%d\n", b, c);
  return 0;
}
