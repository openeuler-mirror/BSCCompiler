// ./run_asan.sh test4.c
#include <stdlib.h>

void test(int m) {
  int *d = (int *)malloc(m * sizeof(int));
  d[m] = 1;
}
int main() {  
  test(100);
  return 0;
}
