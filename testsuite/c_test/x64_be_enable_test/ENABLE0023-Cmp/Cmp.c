int testSigned() {
  int a = 3, b = 4;
  int c = (a == b); // 0
  int d = (a != b); // 1
  int e = (a >= b); // 0
  int f = (a > b);  // 0
  int g = (a <= b); // 1
  int h = (a < b);  // 1
  printf("%d,%d,%d\n", c, d, e);
  printf("%d,%d,%d\n", f, g, h);
  return 0;
}

int testUnsigned() {
  unsigned int a = 3, b = 4;
  unsigned int c = (a == b); // 0
  unsigned int d = (a != b); // 1
  unsigned int e = (a >= b); // 0
  unsigned int f = (a > b);  // 0
  unsigned int g = (a <= b); // 1
  unsigned int h = (a < b);  // 1
  printf("%u,%u,%u\n", c, d, e);
  printf("%u,%u,%u\n", f, g, h);
  return 0;
}

int main() {
  testSigned();
  testUnsigned();
  return 0;
}