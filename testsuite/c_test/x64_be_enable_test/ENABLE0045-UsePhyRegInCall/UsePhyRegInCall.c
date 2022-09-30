void testShift() {
  int a = 1, b = 2, c = 3, d = 4, e = 5;
  printf("%x,%x,%x,%x\n", a << b, a << c, a << d, a << e);
}

void testDiv() {
  int a = 100, b = 2, c = 3, d = 4, e = 5;
  printf("%d,%d,%d,%d\n", a / b, a / c, a / d, a / e);
}

void testRem() {
  int a = 101, b = 2, c = 3, d = 4, e = 5;
  printf("%d,%d,%d,%d\n", a % b, a % c, a % d, a % e);
}

int main() {
  testShift();
  testDiv();
  testRem();
  return 0;
}