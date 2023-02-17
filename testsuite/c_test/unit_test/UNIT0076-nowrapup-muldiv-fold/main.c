#include <limits.h>
#include <stdio.h>

extern void abort ();

int test(int x)
{
  return (2*x)/2;
}

int
main()
{
  int x = INT_MAX;

  if (test(x) != x)
    printf("%x, %x", test(x), x);
    //abort ();
  return 0;
}
