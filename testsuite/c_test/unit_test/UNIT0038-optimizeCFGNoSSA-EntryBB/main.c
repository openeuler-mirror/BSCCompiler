#include "stdio.h"
int c() {
  return rand();
}

void a() {
b:
  for (;;)
    if (c())
      goto b;
}


int main() {
  return 0;
}
