// CHECK: [[# FILENUM:]] "{{.*}}/HelloWorld.c"
#include<stdbool.h>

typedef char CHAR;
typedef short SHORT;
typedef int INT;
typedef long LONG;
typedef float FLOAT;
typedef unsigned int UINT;
typedef double DOUBLE;
typedef signed char SCHAR;
typedef long double LDOUBLE;
struct XX {
  CHAR c;
  SCHAR sc;
  SHORT s;
  INT i;
  unsigned char uc;
  unsigned short us;
  UINT ui;
  LONG l;
  unsigned long ul;
  FLOAT f;
  DOUBLE d;
  LDOUBLE ld;
};

struct XX funcB() {
    struct XX a;
    // CHECK: LOC {{.*}} 32 5
    // CHECK-NEXT:   iassign <* <$XX>> 0 (dread ptr %first_arg_return, dread agg %a_29_15)
    return a;
}

struct XX funcA() {
    // CHECK: LOC {{.*}} 38
    // CHECK-NEXT:   return ()
}

struct XX funcC(bool flag) {
  int i = 1;
  if (flag) {
    struct XX a;
    // CHECK: LOC {{.*}} 46 5
    // CHECK-NEXT:     iassign <* <$XX>> 0 (dread ptr %first_arg_return, dread agg %a_43_15)
    return a;
  } else {
    struct XX b;
    // CHECK: LOC {{.*}} 51 5
    // CHECK-NEXT:     iassign <* <$XX>> 0 (dread ptr %first_arg_return, dread agg %b_48_15)
    return b;
  }
  if (i) {
    struct XX c;
    // CHECK: LOC {{.*}} 57 5
    // CHECK-NEXT:     iassign <* <$XX>> 0 (dread ptr %first_arg_return, dread agg %c_54_15)
    return c;
  }
  switch (i) {
    case 0:{
      struct XX d;
      // CHECK: @label1 LOC {{.*}} 64 7
      // CHECK-NEXT:   iassign <* <$XX>> 0 (dread ptr %first_arg_return, dread agg %d_61_17)
      return d;
    }
  }
  while(i) {
    struct XX e;
    // CHECK: LOC {{.*}} 71 5
    // CHECK-NEXT:     iassign <* <$XX>> 0 (dread ptr %first_arg_return, dread agg %e_68_15)
    return e;
  }
  do
  {
    struct XX f;
    // CHECK: LOC {{.*}} 78 5
    // CHECK-NEXT:     iassign <* <$XX>> 0 (dread ptr %first_arg_return, dread agg %f_75_15)
    return f;
  } while(i);
  // CHECK: LOC {{.*}} 82
  // CHECK-NEXT:   return ()
}

int main() {
  struct XX tmp = funcA();
  return 0;
}
