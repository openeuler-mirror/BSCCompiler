#include <math.h>
#include <float.h>
#include <stdio.h>

int main (void)
{
  int result = 0;
  double local_FLT_MIN = FLT_MIN;
  if (local_FLT_MIN <= 0.0)
    result |= 1;
  printf("result = %d\n", result);
  return 0;
}