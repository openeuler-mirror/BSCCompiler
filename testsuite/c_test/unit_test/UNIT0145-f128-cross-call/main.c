#include <float.h>
#include <math.h>
#include <stdio.h>

long double power(int, int);
long double radpow(int);

int main() {
  float x, y, z;
  x = FLT_MAX / FLT_RADIX;
  y = x + radpow(FLT_MAX_EXP - FLT_MANT_DIG - 1);
  z = radpow(FLT_MAX_EXP - 1);
  if (y != z) {
    printf("error %f != %f\n", y, z);
    return 1;
  }
  return 0;
}

long double power(int base, int n) {
  int i;
  long double x = 1;

  if (n >= 0)
    for (i = 0; i < n; ++i) x *= base;
  else
    for (i = 0; i < -n; ++i) x /= base;
  return (x);
}
/*---------------------------------------------------------------------*\
 * return FLT_RADIX to power n should be exact,                        *
 *  if no overflow or underflow.                                       *
\*---------------------------------------------------------------------*/
long double radpow(int n) {
  return power(FLT_RADIX, n);
}
