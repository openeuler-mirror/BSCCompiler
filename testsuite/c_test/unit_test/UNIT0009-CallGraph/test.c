#include <stdio.h>
#include <stdlib.h>

int max(int a, int b) {
  return (a > b ? a : b);
}
int min(int a, int b) {
  return (a < b ? a : b);
}

void populate_array(int *array, size_t arraySize, int (*getNextValue)(void)) {
  for (size_t i = 0; i < arraySize; i++) {
    array[i] = getNextValue();
  }
}

int getNextRandomValue(void) {
  return rand();
}

typedef struct T {
  int (*func1)(int, int);
  int (*func2)(int, int);
} T;

const T t = {min, max};

int (*const gConsts1[][2])(int, int) = {{min, max}, {max, min}};
int (*const gConsts2[2])(int, int) = {min, max};
int (*const gConst)(int, int) = max;
const int (*gVar)(int, int) = max;
int main(int argc, char *argv[]) {
  gVar = min;
  int (*const lConst)(int, int) = min;
  int (*const lConstFuncArray[])(int, int) = {min, max};
  int a = 1, b = 2, c, d, e, f, g, h, i;
  c = gConst(a, b);
  d = lConst(a, b);
  e = gVar(a, b);
  f = gConsts1[0][1](a, b);
  g = gConsts2[1](a, b);
  h = lConstFuncArray[1](a, b);
  i = t.func1(a, b);
  printf("%d\n", c);
  printf("%d\n", d);
  printf("%d\n", e);
  printf("%d\n", f);
  printf("%d\n", g);
  printf("%d\n", h);
  printf("%d\n", i);

  int myarray[10];
  populate_array(myarray, 10, getNextRandomValue);
  return 0;
}
