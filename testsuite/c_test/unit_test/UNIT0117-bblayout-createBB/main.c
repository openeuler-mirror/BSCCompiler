#include "csmith.h"
int32_t a, c;
int32_t *volatile b;
uint8_t d() {
  for (;; a = 1) {
    if (*b)
      continue;
    if (*b)
      break;
  }
  return c;
}
int main() {}    
