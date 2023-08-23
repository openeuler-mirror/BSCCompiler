#include <stdio.h>

typedef struct {
  unsigned int : 28;
  unsigned int : 30;
} S0;

typedef struct {
  S0 a;
  unsigned short b;
  unsigned int c : 17;
} S1;

S1 s1 = {{}, 62620, 107956U};

int main() {
  printf("%d, %d\n", s1.b, s1.c);
  return 0;
}
