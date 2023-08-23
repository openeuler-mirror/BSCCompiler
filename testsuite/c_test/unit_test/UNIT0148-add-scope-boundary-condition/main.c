//#define uint_32 int
#include "head.h"
int main() {
  int sum = 0;
  int i = 3;

  BEGIN
  {
    int i = 4;
    sum += i;
  }
  END

  sum += i;
  return 0;

}
