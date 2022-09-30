#include <stdio.h>
int feq (){
  int a = 1, b = 2;
  if (a == b)
    return 10;
  else
    return 20;
}

int main() {
  printf("res = %d\n", feq());
}