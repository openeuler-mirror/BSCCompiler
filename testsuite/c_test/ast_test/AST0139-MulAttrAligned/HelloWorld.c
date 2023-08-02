#include<stdint.h>
union A {
  volatile int16_t f0;
  const uint16_t f1;
  const unsigned f2 : 9;
  uint32_t f3;
  volatile int16_t f4;
} __attribute__((aligned(32))) __attribute__((aligned(1)));

struct B {
 int64_t a;
 uint32_t  __attribute__((aligned(128))) __attribute__((aligned(64))) f3 : 1;
 int32_t c;
};

union C {
  volatile int16_t f0;
  const uint16_t f1;
  const unsigned f2 : 9;
  uint32_t f3;
  volatile int16_t f4;
} __attribute__((aligned(1))) __attribute__((aligned(32)));

int main() {
  struct B stu;
  printf("%d\n", sizeof(union A));
  printf("%d\n", sizeof(struct B));
  printf("%d", sizeof(union C));
  return 0;
}
