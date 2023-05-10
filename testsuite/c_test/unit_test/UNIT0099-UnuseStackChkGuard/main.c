#include <stdio.h>
#include <string.h>

struct StructReturnTypeOneInt {
  int a1;
};

struct StructReturnTypeOneInt FuncTestStructReturnTypeOneInt(void);

__attribute__((noinline)) int FuncTestStructReturnTypeOneInt1() {
  int res = FuncTestStructReturnTypeOneInt().a1;
  return res;
}

__attribute__((noinline)) int FuncTestStructReturnTypeOneInt2() {
  struct StructReturnTypeOneInt res = FuncTestStructReturnTypeOneInt();
  return res.a1;
}

struct StructReturnTypeTwoInt {
  int a1, a2;
};

struct StructReturnTypeTwoInt FuncTestStructReturnTypeTwoInt(void);

__attribute__((noinline)) int FuncTestStructReturnTypeTwoInt1() {
  int res = FuncTestStructReturnTypeTwoInt().a1;
  return res;
}

__attribute__((noinline)) int FuncTestStructReturnTypeTwoInt2() {
  struct StructReturnTypeTwoInt res = FuncTestStructReturnTypeTwoInt();
  return res.a1;
}

struct StructReturnTypeThreeInt {
  int a1, a2, a3;
};

struct StructReturnTypeThreeInt FuncTestStructReturnTypeThreeInt(void);

__attribute__((noinline)) int FuncTestStructReturnTypeThreeInt1() {
  int res = FuncTestStructReturnTypeThreeInt().a1;
  return res;
}

__attribute__((noinline)) int FuncTestStructReturnTypeThreeInt2() {
  struct StructReturnTypeThreeInt res = FuncTestStructReturnTypeThreeInt();
  return res.a1;
}

struct StructReturnTypeFourInt {
  int a1, a2, a3, a4;
};

struct StructReturnTypeFourInt FuncTestStructReturnTypeFourInt(void);

__attribute__((noinline)) int FuncTestStructReturnTypeFourInt1() {
  int res = FuncTestStructReturnTypeFourInt().a1;
  return res;
}

__attribute__((noinline)) int FuncTestStructReturnTypeFourInt2() {
  struct StructReturnTypeFourInt res = FuncTestStructReturnTypeFourInt();
  return res.a1;
}

struct StructReturnTypeOneFloat {
  float a1;
};

struct StructReturnTypeOneFloat FuncTestStructReturnTypeOneFloat(void);

__attribute__((noinline)) float FuncTestStructReturnTypeOneFloat1() {
  float res = FuncTestStructReturnTypeOneFloat().a1;
  return res;
}

__attribute__((noinline)) float FuncTestStructReturnTypeOneFloat2() {
  struct StructReturnTypeOneFloat res = FuncTestStructReturnTypeOneFloat();
  return res.a1;
}

struct StructReturnTypeTwoFloat {
  float a1, a2;
};

struct StructReturnTypeTwoFloat FuncTestStructReturnTypeTwoFloat(void);

__attribute__((noinline)) float FuncTestStructReturnTypeTwoFloat1() {
  float res = FuncTestStructReturnTypeTwoFloat().a1;
  return res;
}

__attribute__((noinline)) float FuncTestStructReturnTypeTwoFloat2() {
  struct StructReturnTypeTwoFloat res = FuncTestStructReturnTypeTwoFloat();
  return res.a1;
}

struct StructReturnTypeThreeFloat {
  float a1, a2, a3;
};

struct StructReturnTypeThreeFloat FuncTestStructReturnTypeThreeFloat(void);

__attribute__((noinline)) float FuncTestStructReturnTypeThreeFloat1() {
  float res = FuncTestStructReturnTypeThreeFloat().a1;
  return res;
}

__attribute__((noinline)) float FuncTestStructReturnTypeThreeFloat2() {
  struct StructReturnTypeThreeFloat res = FuncTestStructReturnTypeThreeFloat();
  return res.a1;
}

struct StructReturnTypeFourFloat {
  float a1, a2, a3, a4;
};

struct StructReturnTypeFourFloat FuncTestStructReturnTypeFourFloat(void);

__attribute__((noinline)) float FuncTestStructReturnTypeFourFloat1() {
  float res = FuncTestStructReturnTypeFourFloat().a1;
  return res;
}

__attribute__((noinline)) float FuncTestStructReturnTypeFourFloat2() {
  struct StructReturnTypeFourFloat res = FuncTestStructReturnTypeFourFloat();
  return res.a1;
}

int main() {
  return 0;
}

// CHECK-NOT: __stack_chk_guard