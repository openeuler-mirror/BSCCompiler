#include "csmith.h"
#include "stdalign.h"
#pragma pack(1)
struct S0 {
  const signed f0;
  signed : 0;
  const volatile signed f1;
  volatile signed f2;
  int8_t f3;
  signed f4;
};

struct S1 {
  const signed f0;
  signed : 1;
  const volatile signed f1;
  volatile signed f2;
  int8_t f3;
  signed f4;
};

struct S2 {
  const signed f0;
  const volatile signed f1;
  volatile signed f2;
  int8_t f3;
  signed f4;
};
#pragma pack()

struct __attribute__((packed)) S3 {
  unsigned int a:3;
  unsigned int :0;
  unsigned int b:7;
  unsigned long c:21;
};

struct __attribute__((packed)) S4 {
  unsigned int a:3;
  unsigned int :8;
  unsigned int b:7;
  unsigned long c:21;
};

int main() {
  struct S0 s0;
  struct S1 s1;
  struct S2 s2;
  struct S3 s3;
  struct S4 s4;
  printf("S0 size:  = %lu\n", sizeof(struct S0)); // 20
  printf("S0 align: = %lu\n", alignof(struct S0)); // 4
  printf("S1 size:  = %lu\n", sizeof(struct S1)); // 18
  printf("S1 align: = %lu\n", alignof(struct S1)); // 1
  printf("S2 size:  = %lu\n", sizeof(struct S2)); // 17
  printf("S2 align: = %lu\n", alignof(struct S2)); // 1
  printf("S3 size:  = %lu\n", sizeof(struct S3)); // 8
  printf("S3 align: = %lu\n", alignof(struct S3)); // 4
  printf("S4 size:  = %lu\n", sizeof(struct S4)); // 5
  printf("S4 align: = %lu\n", alignof(struct S4)); // 1
}