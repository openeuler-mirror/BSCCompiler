// ./run_asan.sh test2.c
#include <stdio.h>

int main() {
  char d[20];
  d[21] = 1;
  printf("%d\n", d[11]);
  return 0;
}
