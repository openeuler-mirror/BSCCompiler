#include <stdint.h>

// CHECK: [[# FILENUM:]] "{{.*}}/FirstargReturnTypeAttribute.c"
struct A;
struct Z;
struct X;
typedef struct A *a;
typedef struct Z *z;
typedef struct X *x;
typedef void (*B)(a a_h);
typedef void (*W)(z z_h);
typedef void (*O)(x x_h);

typedef struct S_h {
  B b;
  uint64_t q;
  uint32_t w;
  uint64_t e;
  uint64_t r;
} C;

typedef struct {
  C *c;
} D;

typedef struct {
  D d;
} E;

typedef struct A {
  E e;
  uint64_t q : 31;
  uint32_t w : 31;
  uint64_t p : 31;
  uint64_t r : 31;
} F;

typedef enum S_i {
  p = 0
} G;

typedef struct H {
  W w;
  uint32_t q : 31;
} I;

typedef struct J {
  O o;
  uint64_t p : 31;
} K;

inline static F FuncA(void) {
  return (F){0};
}

inline static I FuncB(void) {
  return (I){0};
}

inline static K FuncC(void) {
  return (K){0};
}

inline static void Test(F *f, I *i, K *k, G g) {
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 4 ]]
  ((void)(f));
  if (f->e.d.c[g].b != ((void *)0)) {
    // CHECK :     icallproto <func (<* <$A>>) void> (
    f->e.d.c[g].b(f);
  }
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 1 ]]
  i->w(i);
  // CHECK :  icallproto <func (<* <$Z>>) void> (
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 1 ]]
  k->o(k);
  // CHECK :  icallproto <func (<* <$X>>) void> (
}

int main() {
  F *f;
  I *i;
  K *k;
  G g;
  Test(f, i, k, g);
}