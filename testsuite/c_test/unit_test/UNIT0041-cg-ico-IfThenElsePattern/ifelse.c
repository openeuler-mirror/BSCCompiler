#include <stdio.h>

unsigned char a, c;
unsigned int b;
signed char d = 127;
void fn1() {
  for (_Bool h = 0; h < 1; h += 1) {
    for (short i = 0; i < 3; i += 0 + 3)
      a = (b ? d : c) ? 512 : 9007164895002624 ? 33 : 9007164895002624;
    for (short j = 0; 0 < 0; j = 0)
      for (signed char k = 0; 0 < 0; k = 0)
            ;
  }
}

int main() {
  fn1();
  printf("%u\n", a);
}
