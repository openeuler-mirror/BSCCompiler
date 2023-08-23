#include <stdint.h>

typedef struct {
  uint8_t a[31];
} S31;

void foo_31(S31 *dst, S31 *src) {
  // CHECK: ldr x{{.*}}#23
  // CHECK: str x{{.*}}#23
  // CHECK-NOT: ldr w{{.*}}
  // CHECK-NOT: str w{{.*}}
  // CHECK-NOT: ldrh w{{.*}}
  // CHECK-NOT: strh w{{.*}}
  // CHECK-NOT: ldrb w{{.*}}
  // CHECK-NOT: strb w{{.*}}
  *dst = *src;
}

typedef struct {
  uint8_t a[30];
} S30;

void foo_30(S30 *dst, S30 *src) {
  // CHECK: ldr x{{.*}}#22
  // CHECK: str x{{.*}}#22
  // CHECK-NOT: ldr w{{.*}}
  // CHECK-NOT: str w{{.*}}
  // CHECK-NOT: ldrh w{{.*}}
  // CHECK-NOT: strh w{{.*}}
  *dst = *src;
}

typedef struct {
  uint8_t a[29];
} S29;

void foo_29(S29 *dst, S29 *src) {
  // CHECK: ldr x{{.*}}#21
  // CHECK: str x{{.*}}#21
  // CHECK-NOT: ldr w{{.*}}
  // CHECK-NOT: str w{{.*}}
  // CHECK-NOT: ldrb w{{.*}}
  // CHECK-NOT: strb w{{.*}}
  *dst = *src;
}

typedef struct {
  uint8_t a[27];
} S27;

void foo_27(S27 *dst, S27 *src) {
  // CHECK: ldr w{{.*}}#23
  // CHECK: str w{{.*}}#23
  // CHECK-NOT: ldrh w{{.*}}
  // CHECK-NOT: strh w{{.*}}
  // CHECK-NOT: ldrb w{{.*}}
  // CHECK-NOT: strb w{{.*}}
  *dst = *src;
}

typedef struct {
  uint8_t a[23];
} S23;

void foo_23(S23 *dst, S23 *src) {
  // CHECK: ldr x{{.*}}#15
  // CHECK: str x{{.*}}#15
  // CHECK-NOT: ldr w{{.*}}
  // CHECK-NOT: str w{{.*}}
  // CHECK-NOT: ldrh w{{.*}}
  // CHECK-NOT: strh w{{.*}}
  // CHECK-NOT: ldrb w{{.*}}
  // CHECK-NOT: strb w{{.*}}
  *dst = *src;
}

typedef struct {
  uint8_t a[22];
} S22;

void foo_22(S22 *dst, S22 *src) {
  // CHECK: ldr x{{.*}}#14
  // CHECK: str x{{.*}}#14
  // CHECK-NOT: ldr w{{.*}}
  // CHECK-NOT: str w{{.*}}
  // CHECK-NOT: ldrh w{{.*}}
  // CHECK-NOT: strh w{{.*}}
  *dst = *src;
}

typedef struct {
  uint8_t a[21];
} S21;

void foo_21(S21 *dst, S21 *src) {
  // CHECK: ldr x{{.*}}#13
  // CHECK: str x{{.*}}#13
  // CHECK-NOT: ldr w{{.*}}
  // CHECK-NOT: str w{{.*}}
  // CHECK-NOT: ldrb w{{.*}}
  // CHECK-NOT: strb w{{.*}}
  *dst = *src;
}

typedef struct {
  uint8_t a[19];
} S19;

void foo_19(S19 *dst, S19 *src) {
  // CHECK: ldr w{{.*}}#15
  // CHECK: str w{{.*}}#15
  // CHECK-NOT: ldrh w{{.*}}
  // CHECK-NOT: strh w{{.*}}
  // CHECK-NOT: ldrb w{{.*}}
  // CHECK-NOT: strb w{{.*}}
  *dst = *src;
}

typedef struct {
  uint8_t a[15];
} S15;

void foo_15(S15 *dst, S15 *src) {
  // CHECK: ldr x{{.*}}#7
  // CHECK: str x{{.*}}#7
  // CHECK-NOT: ldr w{{.*}}
  // CHECK-NOT: str w{{.*}}
  // CHECK-NOT: ldrh w{{.*}}
  // CHECK-NOT: strh w{{.*}}
  // CHECK-NOT: ldrb w{{.*}}
  // CHECK-NOT: strb w{{.*}}
  *dst = *src;
}

typedef struct {
  uint8_t a[14];
} S14;

void foo_14(S14 *dst, S14 *src) {
  // CHECK: ldr x{{.*}}#6
  // CHECK: str x{{.*}}#6
  // CHECK-NOT: ldr w{{.*}}
  // CHECK-NOT: str w{{.*}}
  // CHECK-NOT: ldrh w{{.*}}
  // CHECK-NOT: strh w{{.*}}
  *dst = *src;
}

typedef struct {
  uint8_t a[13];
} S13;

void foo_13(S13 *dst, S13 *src) {
  // CHECK: ldr x{{.*}}#5
  // CHECK: str x{{.*}}#5
  // CHECK-NOT: ldr w{{.*}}
  // CHECK-NOT: str w{{.*}}
  // CHECK-NOT: ldrb w{{.*}}
  // CHECK-NOT: strb w{{.*}}
  *dst = *src;
}

typedef struct {
  uint8_t a[11];
} S11;

void foo_11(S11 *dst, S11 *src) {
  // CHECK: ldr w{{.*}}#7
  // CHECK: str w{{.*}}#7
  // CHECK-NOT: ldrh w{{.*}}
  // CHECK-NOT: strh w{{.*}}
  // CHECK-NOT: ldrb w{{.*}}
  // CHECK-NOT: strb w{{.*}}
  *dst = *src;
}

int main() {
  return 0;
}