// -std=c99
// All functions starts with 'd' should be deleted from the module.
// All functions starts with 'e' is externally visible, should not be deleted.

// extern decl (explicit) + inline def
extern int efunc1(int, int);
inline int efunc1(int x, int y) {
  return x + y;
}

// implicit extern decl + inline def
int efunc2(int, int);
inline int efunc2(int x, int y) {
  return x + y;
}

// explicit extern inline decl + inline def
extern inline int efunc3(int, int);
inline int efunc3(int x, int y) {
  return x + y;
}

// inline def + explicit extern decl
inline int efunc4(int x, int y) {
  return x + y;
}
extern int efunc4(int, int);

// inline def + implicit extern decl
inline int efunc5(int x, int y) {
  return x + y;
}
int efunc5(int, int);

// inline def + explicit extern inline decl
extern inline int efunc6(int, int);
inline int efunc6(int x, int y) {
  return x + y;
}

// block scope decl + inline def
void bar() {
  extern int efunc7(int, int);
}
inline int efunc7(int x, int y) {
  return x + y;
}

// gnu_inline
// The definition is used for inlining when possible. It is compiled as
// a standalone function (emitted as a strong definition) and emitted with external linkage.
__attribute__((gnu_inline)) inline int efunc8(int x, int y) {
  return x + y;
}

// extern gnu_inline
extern __attribute__((gnu_inline)) inline int dfunc3(int x, int y) {
  return x + y;
}

// only inline def
inline int dfunc1(int x, int y) {
  return x + y;
}

// inline def + block scope decl
inline int dfunc2(int x, int y) {
  return x + y;
}
void foo() {
  extern int dfunc2(int, int);
}

int main() {
  int ret = dfunc1(1, 2) + efunc8(1, 1);
  return ret;
}

