void testChar() {
  char a = 9, b = 6;
  unsigned char c = 5, d = 3;
  char m = a - b;
  unsigned char n = c - d;
  printf("%d,%u\n", m, n);
}

void testShort() {
  short a = 9, b = 6;
  unsigned short c = 5, d = 3;
  short m = a - b;
  unsigned short n = c - d;
  printf("%d,%u\n", m, n);
}

void testInt() {
  int a = 9, b = 6;
  unsigned int c = 5, d = 3;
  int m = a - b;
  unsigned int n = c - d;
  printf("%d,%u\n", m, n);
}

void testLong() {
  long long a = 9, b = 6;
  unsigned long long c = 5, d = 3;
  long long m = a - b;
  unsigned long long n = c - d;
  printf("%lld,%llu\n", m, n);
}

int main() {
  testChar();
  testShort();
  testInt();
  testLong();
  return 0;
}
