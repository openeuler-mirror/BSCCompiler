
#include <stdio.h>
#include <math.h>
int main ()
{
  printf("%d\n", isless(1.0, NAN));
  printf("%d\n", islessequal(1.0, NAN));
  printf("%d\n", isgreater(1.0, NAN));
  printf("%d\n", isgreaterequal(1.0, NAN));
  printf("%d\n", isless(NAN, 1.0));
  printf("%d\n", islessequal(NAN, 1.0));
  printf("%d\n", isgreater(NAN, 1.0));
  printf("%d\n", isgreaterequal(NAN, 1.0));
  return 0;
}

