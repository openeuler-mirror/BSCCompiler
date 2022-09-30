#include <stdio.h>
int g_a[5] = {0,1,2,3,4};
int b = 12;

int main() {
  // auto
  int a = 1;
  int b = a + 1;
  printf("b = %d\n", b);
  int *c = &a;
  printf("*c = %d\n", *c);
  // static is unsupported well now.
  // global
  int *b2 = g_a;
  printf("*ga[2] = %d\n", *(b2 + 2));
}
