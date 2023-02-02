#include <stdio.h>
int main (int argc, char **argv)
{
  // int x = 8;
  // int y = __builtin_ctz(x);
  // printf("%d\n", y);

  // long a = 14;
  // long b = __builtin_ctzl(a);
  // printf("%l\n", b);

  long long c = 16;
  long long d = __builtin_ctzll(c);
  printf("%ll\n", d);
  return 0;
}