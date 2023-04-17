// ./run_asan.sh test3.c
#include <stdlib.h>

void test(int a) {
  if (a) {
    int *d = (int *)alloca(100 * sizeof(int));
    d[101] = 1;
  }
}
int main() {  
  test(1);
  return 0;
}
