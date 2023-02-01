#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>

void test() {
  int ptr = 2;
  long expected = 3;
  int desired = 6;
  // CHECK-NOT: [[# @LINE + 1 ]] error:size mismatch in argument 2 of '__atomic_compare_exchange'
  __atomic_compare_exchange(&ptr, &expected, desired, true, __ATOMIC_RELEASE, __ATOMIC_RELAXED);
}

void test1() {
  int ptr = 2;
  char expected = 3;
  int desired = 6;
  // CHECK: [[# @LINE + 1]] error: size mismatch in argument 2 of '__atomic_compare_exchange'
  __atomic_compare_exchange(&ptr, &expected, &desired, true, __ATOMIC_RELEASE, __ATOMIC_RELAXED);
}

void test2() {
  int ptr = 2;
  int expected = 3;
  long desired = 6;
  // CHECK: [[# @LINE + 1]] error: size mismatch in argument 3 of '__atomic_compare_exchange'
  __atomic_compare_exchange(&ptr, &expected, &desired, true, __ATOMIC_RELEASE, __ATOMIC_RELAXED);
}

int main() {
  return 0;
}
