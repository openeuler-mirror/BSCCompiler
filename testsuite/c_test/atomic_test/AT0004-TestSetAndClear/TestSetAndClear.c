#include<stdio.h>

void test1() {
  printf("---test1---\n");
  _Bool ptr = 0;
  _Bool val = __atomic_test_and_set(&ptr, __ATOMIC_RELAXED);
   printf("ptr=%d, val=%d\n", ptr, val);

   __atomic_clear(&ptr, __ATOMIC_SEQ_CST);
   printf("ptr=%d, val=%d\n", ptr, val);
}

void test2() {
  printf("---test2---\n");
  int ptr = 12334;
  int val = __atomic_test_and_set(&ptr, __ATOMIC_ACQUIRE);
  printf("ptr=%d, val=%d\n", ptr, val);

  __atomic_clear(&ptr, __ATOMIC_RELEASE);
  printf("ptr=%d, val=%d\n", ptr, val);
}

void test3() {
  printf("---test3---\n");
  int ptr = 9;
  _Bool val = __atomic_test_and_set(&ptr, __ATOMIC_SEQ_CST);
  printf("ptr=%d, val=%d\n", ptr, val);

  __atomic_clear(&ptr, __ATOMIC_RELAXED);
  printf("ptr=%d, val=%d\n", ptr, val);

  _Bool ptr2 = 9;
  int val2 = __atomic_test_and_set(&ptr2, __ATOMIC_ACQ_REL);
  printf("ptr2=%d, val2=%d\n", ptr2, val2);

  __atomic_clear(&ptr2, __ATOMIC_RELAXED);
  printf("ptr2=%d, val2=%d\n", ptr2, val2);
}

void test4() {
  printf("---test4---\n");
  long ptr = 123456789L;
  long val = __atomic_test_and_set(&ptr, __ATOMIC_CONSUME);
  printf("ptr=%ld, val=%ld\n", ptr, val);

  __atomic_clear(&ptr, __ATOMIC_RELAXED);
  printf("ptr=%ld, val=%ld\n", ptr, val);
}

int main() {
  test1();
  test2();
  test3();
  test4();
  return 0;
}


