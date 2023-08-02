#include<stdlib.h>
#include<c_enhanced.h>
#pragma SAFE ON

typedef struct A {
    int* p __attribute__((nonnull));
}A;

BCNTI(3, 1)
SAFE void *ExtFunc(void *__s, int __c, size_t count);

BCNTI(3, 1)
SAFE void *memset(void *__s, int __c, size_t count);


int main() {
  A aa = {(int*)malloc(64)};
  A *p1 = &aa;
  int *p2 = (int*)&aa;
  A a1 = {NULL};               // CHECK: [[# @LINE ]] error: null assignment of nonnull pointer
  memset(p1, 0, sizeof(A));    // CHECK-NEXT: [[# @LINE ]] error: null assignment of nonnull structure field pointer in memset
                               // CHECK-NEXT: [[# @LINE - 1]] error: boundaryless pointer passed to memset that requires a boundary pointer for the 1st argument
  aa.p = NULL;                 // CHECK-NEXT: [[# @LINE ]] error: null assignment of nonnull pointer
  memset(p2, 0, sizeof(A));    // CHECK-NEXT: [[# @LINE ]] error: boundaryless pointer passed to memset that requires a boundary pointer for the 1st argument
  ExtFunc(&aa, 0, sizeof(A));
  memset(&aa, 0, sizeof(A));   // CHECK-NEXT: [[# @LINE ]] error: null assignment of nonnull structure field pointer in memset
                               // CHECK-NEXT: Info  00: 6 error(s) generated.
  return 0;
}
