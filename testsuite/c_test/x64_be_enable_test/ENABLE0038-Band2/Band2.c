void testChar() {
  char a = 4, b = 12;
  unsigned char c = 4, d = 12;
  char m = a & b;
  unsigned char n = c & d;
  printf("%d,%u\n", m, n);
}

void testShort() {
  short a = 4, b = 12;
  unsigned short c = 4, d = 12;
  short m = a & b;
  unsigned short n = c & d;
  printf("%d,%u\n", m, n);
}

void testInt() {
  int a = 4, b = 12;
  unsigned int c = 4, d = 12;
  int m = a & b;
  unsigned int n = c & d;
  printf("%d,%u\n", m, n);
}

void testLong() {
  long long a = 4, b = 12;
  unsigned long long c = 4, d = 12;
  long long m = a & b;
  unsigned long long n = c & d;
  printf("%lld,%llu\n", m, n);
}

int main() {
  testChar();
  testShort();
  testInt();
  testLong();
  return 0;
}
