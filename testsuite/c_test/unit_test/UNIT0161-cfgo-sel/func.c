#include <stdio.h>


int b = 3;
int c = 0;
__attribute__((noinline)) void foo() {
  int a = 0 < b;
  b = 0;
  c = 0 || a;
}

int main(){
  foo();
  printf("%d\n", c);
}
