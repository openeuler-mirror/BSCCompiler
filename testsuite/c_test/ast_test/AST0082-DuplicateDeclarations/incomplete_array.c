#include <stdio.h>
#include <stdint.h>
#include "incomplete_array.h"

typedef enum {
  kFirst,
  kSecond,
  kThird,
  kFourth
}foo;

int bar[kFourth];  // CHECK: var $bar <[3] i32> used

int main() {
  bar[1] = 1;
  return 0;
}
