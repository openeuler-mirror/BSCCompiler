#include <limits.h>

void IntminOverflow(double *a, double *b, int len) {
  for (int i = INT_MIN; i < INT_MIN + len; i++) {  // unexpected to trans to `len - INT_MIN`
    a[i - INT_MIN] += b[i - INT_MIN];
  }
}

void Init(double *a, int len, int flag) {
  for (int i = 0; i < len; i++) {
    a[i] = i + i * 0.5 + flag;
  }
}

double a[100];
double b[100];

int main() {
  for (int i = 0; i < 100; i++) {
    Init(&a, i, 2);
    Init(&b, i, 5);
    IntminOverflow(&a, &b, i);
  }
  return 0;
}
