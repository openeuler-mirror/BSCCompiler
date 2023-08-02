#include <limits.h>

void ConditionalOverflow(double *a, double *b, int len) {
  if (len >= 1) {
    for (int i = INT_MIN; i < INT_MIN + (len - 1); i++) {
        a[i - INT_MIN] += b[i - INT_MIN];
    }
  }
}

void Init(double *a, int len, int fix) {
  for (int i = 0; i < len; i++) {
    a[i] = i + i * 0.5 + fix;
  }
}

double a[100];
double b[100];

int main() {
  for (int i = 0; i < 100; i++) {
    Init(&a, i, 2);
    Init(&b, i, 5);
    ConditionalOverflow(&a, &b, i);
  }
  return 0;
}

