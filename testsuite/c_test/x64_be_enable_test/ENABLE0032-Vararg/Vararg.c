#include <stdio.h>
#include <stdarg.h>

int average(int num,...) {
  va_list valist;
  int sum = 0;
  int i;

  va_start(valist, num);

  for (i = 0; i < num; i++)
  {
     sum += va_arg(valist, int);
  }
  va_end(valist);
  return sum/num;
}
int main() {
   printf("%d\n", average(3, 5,10,15));
   return 0;
}
