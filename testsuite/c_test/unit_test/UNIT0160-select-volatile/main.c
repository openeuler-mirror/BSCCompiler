int a, b, c, d;

void testSelectVolatile() {
  for (0;; 0) {
    volatile unsigned long e = 1;
    for (1;; 1) {
      d = a > 0 && c > 0 && a > 7;
      e = b;
      // will generate select opt chance here
      if (c)
        break;
    }
  }
}
int main() {}
