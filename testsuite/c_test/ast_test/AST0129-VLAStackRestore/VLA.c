// CHECK: [[# FILENUM:]] "{{.*}}/VLA.c"
#include <stdio.h>
extern void abort (void);

struct A {
  int a;
  int b;
  void* c;
};
void Test(int n){
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK-NOT: intrinsiccallassigned C_stack_save
  char i[n];
  struct A m = { {1,2,NULL}, .a=1, .c=__builtin_alloca (16)};
  // CHECK-NOT: intrinsiccall C_stack_restore
}

int foo (int n) {
  char *p, *q;

  if (1) {
    // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
    // CHECK-NOT: intrinsiccallassigned C_stack_save
    char i[n];
    p = __builtin_alloca (8);
    p[0] = 1;
    printf("p[0]=%d\n", p[0]);
    // CHECK-NOT: intrinsiccall C_stack_restore
  }
  if (0) {
    // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
    // CHECK-NOT: intrinsiccallassigned C_stack_save
    char k[n];
    for (int i = 0; i < 10; i++, __builtin_alloca (8)) {
      // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
      // CHECK: intrinsiccallassigned C_stack_save
      char m[n];
      // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
      // CHECK: intrinsiccall C_stack_restore
    }
    // CHECK-NOT: intrinsiccall C_stack_restore
  }

  q = __builtin_alloca (64);
  __builtin_memset (q, 0, 64);

  printf("p[0]_2=%d\n", p[0]);
  return !p[0];
}

void func1();
void func2() {
  func1();
}
void func1() {
  int n = 3;
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  // CHECK: dassign %vla_size
  char i[n];
}

int main (void) {
  if (foo (48) != 0) {
    abort ();
  }
  return 0;
}
