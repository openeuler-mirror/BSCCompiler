#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

int main()
{
  volatile int a[10], b[10], c[10], i;

  for (i = 0; i < 10; i++) {
    b[i] = i << 2;
    c[i] = i << (b[i] % (sizeof(int)*CHAR_BIT));
  }

  for (i = 0; i < 10; i++) {
    a[i] = b[i] + c[9-i];
  }

  int sum = 0;
  for (i = 0; i < 10; i++) {
    sum += a[i];
  }

  if (sum != 1985229660) {
    abort();
  }

  return 0;
}
