#include <limits.h>

static inline void SkipOverflow(long long *a, long long *b, int len) {
  unsigned int i;
  for (i = (INT_MAX - (len)); i < INT_MAX; i++) {
    if ((i - (INT_MAX - (len))) > 32) {
      a[(i - (INT_MAX - (len)))] = a[(i - (INT_MAX - (len)))-1] - 1;
    } else {
      b[(i - (INT_MAX - (len)))] -= (i - (INT_MAX - (len)));
    }
  }
}

long long a[40];
long long b[32];

int main() {
  a[32] = 9;
  SkipOverflow(a, b, 40);
  return 0;
}
