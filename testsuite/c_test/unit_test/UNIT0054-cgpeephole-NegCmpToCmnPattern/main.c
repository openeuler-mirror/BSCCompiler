#include <stdio.h>

unsigned int a = 0;

void func(unsigned int b) {
  unsigned int d = -a;
  if (b < d) {
     printf("%s\n", "false");
  } else {
     printf("%s\n", "true");
  }
}

int main() {
  unsigned int c = 8;
  func(c);
}
