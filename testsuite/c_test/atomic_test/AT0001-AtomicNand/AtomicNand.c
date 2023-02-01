#include<stdio.h>
#include<stdlib.h>

void test() {
  int ptr = 2;
  int val = 1;
  __atomic_fetch_nand(&ptr, val, __ATOMIC_RELAXED);
  printf("ptr=%d, val=%d\n", ptr, val);
  __atomic_nand_fetch(&ptr, val, __ATOMIC_CONSUME);
  printf("ptr=%d, val=%d\n", ptr, val);
  __atomic_nand_fetch(&ptr, val, __ATOMIC_ACQ_REL);
  printf("ptr=%d, val=%d\n", ptr, val);
}

int main() {
  test();
  return 0;
}
