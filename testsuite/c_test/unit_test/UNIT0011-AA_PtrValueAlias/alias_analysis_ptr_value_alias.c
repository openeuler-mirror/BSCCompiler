#include <stdio.h>

extern void abort();
struct MemArrays { char a[4], b[4]; };

int main (void)
{
  const struct MemArrays ma[] = {
    { { '1', '2', '3', '4' }, { '5', '\0', '\0', '\0' } },
  };

  const char *s = ma[0].a;
  unsigned n = strlen (s);
  printf("%d\n", n);
  if (n != 5) {
    abort();
  }
  return 0;
}
