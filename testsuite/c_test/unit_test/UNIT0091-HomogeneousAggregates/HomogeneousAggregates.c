#include <arm_neon.h>

typedef struct {
  float a[2];
  float b;
} S0;
S0 func0() {
  // CHECK: ldr s0
  // CHECK-NEXT: ldr s1
  // CHECK-NEXT: ldr s2
  S0 s;
  return s;
}

typedef struct {
  double a[2];
  double b;
} S1;
S1 func1() {
  // CHECK: ldr d0
  // CHECK-NEXT: ldr d1
  // CHECK-NEXT: ldr d2
  S1 s;
  return s;
}

typedef struct {
  S1 a;
  double b;
} S2;
S2 func2() {
  // CHECK: ldr d0
  // CHECK-NEXT: ldr d1
  // CHECK-NEXT: ldr d2
  // CHECK-NEXT: ldr d3
  S2 s;
  return s;
}

typedef struct __attribute__ ((__packed__)) {
  float a;
} S3;
S3 func3() {
  // CHECK: ldr s0
  S3 s;
  return s;
}

typedef struct {
  S3 a;
  float b;
} S4;
S4 func4() {
  // CHECK: ldr s0
  // CHECK-NEXT: ldr s1
  S4 s;
  return s;
}

typedef struct {
  uint8x8_t a;
  uint16x4_t b;
} S5;
S5 func5() {
  // CHECK: ldr d0
  // CHECK-NEXT: ldr d1
  S5 s;
  return s;
}

typedef struct {
  uint8x16_t a;
  uint16x8_t b;
} S6;
S6 func6() {
  // CHECK: ldr q0
  // CHECK-NEXT: ldr q1
  S6 s;
  return s;
}

typedef struct {} Empty;
typedef struct {
  uint8x16_t a;
  Empty b;
} S7;
S7 func7() {
  // CHECK: ldr q0
  S7 s;
  return s;
}

typedef union {
  double a[2];
  double b;
} S8;
S8 func8() {
  S8 s;
  // CHECK: ldr d0
  // CHECK-NEXT: ldr d1
  return s;
}

int8x8x2_t func9() {
  int8x8x2_t a;
  // CHECK: ldr d0
  // CHECK-NEXT: ldr d1
  return a;
}

typedef struct {
  int8x8x2_t a;
  int8x8_t b;
} S10;
S10 func10() {
  S10 a;
  // CHECK: ldr d0
  // CHECK-NEXT: ldr d1
  // CHECK-NEXT: ldr d2
  return a;
}

typedef union {
  double a[2];
  double b;
  Empty c;
} S11;
S11 func11() {
  S11 s;
  // CHECK: ldr d0
  // CHECK-NEXT: ldr d1
  return s;
}

typedef int v4si __attribute__ ((vector_size (16)));
v4si fucn12() {
  v4si a;
  // CHECK: ldr q0
  return a;
}

typedef struct {
  v4si a, b;
} S13;
S13 func13() {
  S13 s;
  // CHECK: ldr q0
  // CHECK-NEXT: ldr q1
  return s;
}

typedef struct {
  v4si a, b;
  int8x16_t c;
} S14;
S14 func14() {
  S14 s;
  // CHECK: ldr q0
  // CHECK-NEXT: ldr q1
  // CHECK-NEXT: ldr q2
  return s;
}

int main() {
  return 0;
}