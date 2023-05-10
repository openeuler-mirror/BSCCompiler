#include <stdio.h>

#pragma pack(1)
struct S1 {
  short f1 : 10;
  int f2;
  char f5 : 7;
};  // size is 7, passed by one reg

struct S2 {
  long f1;
  struct S1 f3;
};  // size is 15, passed by two reg

struct S3 {
  long f1;
  long f2;
  struct S1 f3;
};  // size is 23, passed by pointer

void printS1(struct S1 s) {
  printf("%d,%d,%d\n", s.f1, s.f2, s.f5);
}

__attribute__((noinline)) struct S1 PassByRegS1(struct S1 s) {
  struct S1 c[20480];
  return s;
}

__attribute__((noinline)) struct S1 PassByStackS1(int a0, int a1, int a2, int a3, int a4, int a5, int a6, int a7,
                                                  struct S1 s) {
  struct S1 c[20480];
  return s;
}

void testS1() {
  struct S1 a[20480];
  struct S1 b = {2, 5, 2};
  b = PassByRegS1(b);
  printS1(b);
  struct S1 c = {5, 6, 3};
  c = PassByStackS1(0, 1, 2, 3, 4, 5, 6, 7, c);
  printS1(c);
}

void printS2(struct S2 s) {
  printf("%ld,", s.f1);
  printS1(s.f3);
}

__attribute__((noinline)) struct S2 PassByRegS2(struct S2 s) {
  struct S2 c[20480];
  return s;
}

__attribute__((noinline)) struct S2 PassByStackS2(int a0, int a1, int a2, int a3, int a4, int a5, int a6, int a7,
                                                  struct S2 s) {
  struct S2 c[20480];
  return s;
}

void testS2() {
  struct S2 a[20480];
  struct S2 b = {2, 5, 2, 1};
  b = PassByRegS2(b);
  printS2(b);
  struct S2 c = {5, 6, 3, 5};
  c = PassByStackS2(0, 1, 2, 3, 4, 5, 6, 7, c);
  printS2(c);
}

void printS3(struct S3 s) {
  printf("%ld,%ld,", s.f1, s.f2);
  printS1(s.f3);
}

__attribute__((noinline)) struct S3 PassByRegS3(struct S3 s) {
  struct S3 c[20480];
  return s;
}

__attribute__((noinline)) struct S3 PassByStackS3(int a0, int a1, int a2, int a3, int a4, int a5, int a6, int a7,
                                                  struct S3 s) {
  struct S3 c[20480];
  return s;
}

void testS3() {
  struct S3 a[20480];
  struct S3 b = {2, 5, 2, 1, 9};
  b = PassByRegS3(b);
  printS3(b);
  struct S3 c = {5, 6, 3, 5, 8};
  c = PassByStackS3(0, 1, 2, 3, 4, 5, 6, 7, c);
  printS3(c);
}

#define TEST_BASE_TYPE(base, passType, printStr)                                                                     \
  __attribute__((noinline)) base PassByReg_##base(base a) {                                                          \
    base c[20480];                                                                                                   \
    return a;                                                                                                        \
  }                                                                                                                  \
  __attribute__((noinline)) base PassByStack_##base(passType a0, passType a1, passType a2, passType a3, passType a4, \
                                                    passType a5, passType a6, passType a7, base a) {                 \
    base c[20480];                                                                                                   \
    return a;                                                                                                        \
  }                                                                                                                  \
  void test_##base() {                                                                                               \
    base a[40960];                                                                                                   \
    base b = (base)(56);                                                                                             \
    b = PassByReg_##base(b);                                                                                         \
    printf(printStr, b);                                                                                             \
    base c = (base)(78);                                                                                             \
    c = PassByStack_##base(0, 1, 2, 3, 4, 5, 6, 7, c);                                                               \
    printf(printStr, c);                                                                                             \
  }

TEST_BASE_TYPE(char, int, "%d\n");
TEST_BASE_TYPE(short, int, "%d\n");
TEST_BASE_TYPE(int, int, "%d\n");
TEST_BASE_TYPE(long, int, "%ld\n");
TEST_BASE_TYPE(float, float, "%.2f\n");
TEST_BASE_TYPE(double, float, "%.2f\n");
#undef TEST_BASE_TYPE

int main() {
  printf("sizeof(struct S1) = %ld\n", sizeof(struct S1));
  printf("sizeof(struct S2) = %ld\n", sizeof(struct S2));
  printf("sizeof(struct S3) = %ld\n", sizeof(struct S3));
  testS1();
  testS2();
  testS3();
#define TEST_BASE_TYPE(base, ...) test_##base()
  TEST_BASE_TYPE(char, int, "%d\n");
  TEST_BASE_TYPE(short, int, "%d\n");
  TEST_BASE_TYPE(int, int, "%d\n");
  TEST_BASE_TYPE(long, int, "%ld\n");
  TEST_BASE_TYPE(float, float, "%.2f\n");
  TEST_BASE_TYPE(double, float, "%.2f\n");
  return 0;
}