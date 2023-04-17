// ./run_asan.sh test1.c
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char **argv) {
  int *array = (int *) malloc(100 * sizeof(int));
  free(array);
  int ret_res = array[argc];
  return ret_res;
  // return aray[argc];  // BOOM
}
