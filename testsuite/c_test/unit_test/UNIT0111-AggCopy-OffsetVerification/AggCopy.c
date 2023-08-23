#include <stdint.h>

typedef struct {
  uint8_t a[31];
} S31;

void foo_31(S31 *dst, S31 *src) {
  // CHECK: ldp
  // CHECK-NEXT: stp
  // CHECK-NEXT: ldr x{{.*}}#16
  // CHECK-NEXT: str x{{.*}}#16
  // CHECK-NEXT: ldr x{{.*}}#23
  // CHECK-NEXT: str x{{.*}}#23
  *dst = *src;
}

typedef struct {
  uint8_t a[30];
} S30;

void foo_30(S30 *dst, S30 *src) {
  // CHECK: ldp
  // CHECK-NEXT: stp
  // CHECK-NEXT: ldr x{{.*}}#16
  // CHECK-NEXT: str x{{.*}}#16
  // CHECK-NEXT: ldr x{{.*}}#22
  // CHECK-NEXT: str x{{.*}}#22
  *dst = *src;
}

typedef struct {
  uint8_t a[29];
} S29;

void foo_29(S29 *dst, S29 *src) {
  // CHECK: ldp
  // CHECK-NEXT: stp
  // CHECK-NEXT: ldr x{{.*}}#16
  // CHECK-NEXT: str x{{.*}}#16
  // CHECK-NEXT: ldr x{{.*}}#21
  // CHECK-NEXT: str x{{.*}}#21
  *dst = *src;
}

typedef struct {
  uint8_t a[27];
} S27;

void foo_27(S27 *dst, S27 *src) {
  // CHECK: ldp
  // CHECK-NEXT: stp
  // CHECK-NEXT: ldr x{{.*}}#16
  // CHECK-NEXT: str x{{.*}}#16
  // CHECK-NEXT: ldr w{{.*}}#23
  // CHECK-NEXT: str w{{.*}}#23
  *dst = *src;
}

typedef struct {
  uint8_t a[23];
} S23;

void foo_23(S23 *dst, S23 *src) {
  // CHECK: ldp
  // CHECK-NEXT: stp
  // CHECK-NEXT: ldr x{{.*}}#15
  // CHECK-NEXT: str x{{.*}}#15
  *dst = *src;
}

typedef struct {
  uint8_t a[22];
} S22;

void foo_22(S22 *dst, S22 *src) {
  // CHECK: ldp
  // CHECK-NEXT: stp
  // CHECK-NEXT: ldr x{{.*}}#14
  // CHECK-NEXT: str x{{.*}}#14
  *dst = *src;
}

typedef struct {
  uint8_t a[21];
} S21;

void foo_21(S21 *dst, S21 *src) {
  // CHECK: ldp
  // CHECK-NEXT: stp
  // CHECK-NEXT: ldr x{{.*}}#13
  // CHECK-NEXT: str x{{.*}}#13
  *dst = *src;
}

typedef struct {
  uint8_t a[19];
} S19;

void foo_19(S19 *dst, S19 *src) {
  // CHECK: ldp
  // CHECK-NEXT: stp
  // CHECK-NEXT: ldr w{{.*}}#15
  // CHECK-NEXT: str w{{.*}}#15
  *dst = *src;
}

typedef struct {
  uint8_t a[15];
} S15;

void foo_15(S15 *dst, S15 *src) {
  // CHECK: ldr x
  // CHECK-NEXT: str x
  // CHECK-NEXT: ldr x{{.*}}#7
  // CHECK-NEXT: str x{{.*}}#7
  *dst = *src;
}

typedef struct {
  uint8_t a[14];
} S14;

void foo_14(S14 *dst, S14 *src) {
  // CHECK: ldr x
  // CHECK-NEXT: str x
  // CHECK-NEXT: ldr x{{.*}}#6
  // CHECK-NEXT: str x{{.*}}#6
  *dst = *src;
}

typedef struct {
  uint8_t a[13];
} S13;

void foo_13(S13 *dst, S13 *src) {
  // CHECK: ldr x
  // CHECK-NEXT: str x
  // CHECK-NEXT: ldr x{{.*}}#5
  // CHECK-NEXT: str x{{.*}}#5
  *dst = *src;
}

typedef struct {
  uint8_t a[11];
} S11;

void foo_11(S11 *dst, S11 *src) {
  // CHECK: ldr x
  // CHECK-NEXT: str x
  // CHECK-NEXT: ldr w{{.*}}#7
  // CHECK-NEXT: str w{{.*}}#7
  *dst = *src;
}

typedef struct {
  uint8_t a[7];
} S7;

void foo_7(S7 *dst, S7 *src) {
  // CHECK: ldr w
  // CHECK-NEXT: str w
  *dst = *src;
}

int main() {
  return 0;
}