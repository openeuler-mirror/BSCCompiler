#include <limits.h>
int fx (x)
     short x;
{
  return x;
}

main ()
{ int res = fx(INT_MAX);
  printf("%d\n", res);
}


