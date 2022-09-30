void testChar() {
  char a = 1, b = 2;
  unsigned char c = 3, d = 4;
  char m = a | b;
  unsigned char n = c | d;
  printf("%d,%u\n", m, n);
}

void testShort() {
  short a = 1, b = 2;
  unsigned short c = 3, d = 4;
  short m = a | b;
  unsigned short n = c | d;
  printf("%d,%u\n", m, n);
}

void testInt() {
  int a = 1, b = 2;
  unsigned int c = 3, d = 4;
  int m = a | b;
  unsigned int n = c | d;
  printf("%d,%u\n", m, n);
}

void testLong() {
  long long a = 1, b = 2;
  unsigned long long c = 3, d = 4;
  long long m = a | b;
  unsigned long long n = c | d;
  printf("%lld,%llu\n", m, n);
}

int main() {
  testChar();
  testShort();
  testInt();
  testLong();
  return 0;
}
