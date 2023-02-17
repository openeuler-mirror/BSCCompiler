#include <math.h>
#include <float.h>
#include <stdio.h>

int main (void)
{
  int result = 0;
  double local_DBL_MIN = DBL_MIN;
  if (local_DBL_MIN >= 0.0)
    result |= 1;
  printf("result = %d\n", result);
  return 0;
}