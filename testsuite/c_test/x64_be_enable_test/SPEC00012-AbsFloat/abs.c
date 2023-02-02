#include <stdio.h>
int main() {
  //f64 abs
  double c = -5.4;
  double d = fabs(c);
  printf("%f\n", d);

  c = 5.4;
  d = fabs(c);
  printf("%f\n", d);

  return 0;
}
