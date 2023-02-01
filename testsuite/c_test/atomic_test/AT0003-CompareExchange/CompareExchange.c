#include<stdio.h>
#include<stdlib.h>
#include <stdbool.h>

void test1() {
  printf("---test1---\n");
  int ptr = 2;
  int expected = 3;
  int desired = 6;
  __atomic_compare_exchange_n(&ptr, &expected, desired, true, __ATOMIC_RELEASE, __ATOMIC_RELAXED);
  printf("ptr=%d, expected=%d, desired=%d\n", ptr, expected, desired);
  __atomic_compare_exchange_n(&ptr, &expected, desired, false, __ATOMIC_RELAXED, __ATOMIC_RELAXED);
  printf("ptr=%d, expected=%d, desired=%d\n", ptr, expected, desired);
  __atomic_compare_exchange(&ptr, &expected, &desired, false, __ATOMIC_SEQ_CST, __ATOMIC_RELAXED);
  printf("ptr=%d, expected=%d, desired=%d\n", ptr, expected, desired);
}

void test2() {
  printf("---test2---\n");
  int ptr = 2;
  char expected = 3;
  int desired = 6;
  __atomic_compare_exchange_n(&ptr, &expected, desired, true, __ATOMIC_RELEASE, __ATOMIC_RELAXED);
  printf("ptr=%d, expected=%d, desired=%d\n", ptr, expected, desired);
}

int main() {
  test1();
  test2();
  return 0;
}
