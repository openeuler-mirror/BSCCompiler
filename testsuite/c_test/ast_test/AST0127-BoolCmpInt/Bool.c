// CHECK: [[# FILENUM:]] "{{.*}}/Bool.c"
#include <stdio.h>

int main() {
  _Bool a = (_Bool)1;
  int d = -749133506;
  // CHECK: cvt i32 u1 (dread u32 %a_5_9)))
  // CHECK: LOC [[# FILENUM]] [[# @LINE + 2 ]]
  int res = d > a;
  printf("%d\n", res);
  return 0;
}
