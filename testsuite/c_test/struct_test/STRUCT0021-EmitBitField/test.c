#include <stdio.h>

typedef struct {
  int : 30;
} S0;

typedef struct {
  int : 31;
  S0 a;
  int b : 30;
  int c : 9;
} S1;

S1 s1 = {{}, 963106212U, 269U};

int main() {
  printf("%d\n", s1.b);
  printf("%d\n", s1.c);
  return 0;
}