#include <stdio.h>
// CHECK: [[# FILENUM:]] "{{.*}}/AddrOfLabel.c"
void bar (void *) {}
int main() {
  // CHECK: @__hereA@{{.*}}
  // CHECK: call &bar (addroflabel ptr @__hereA@{{.*}})
  bar(({__hereA:&&__hereA;}));
  // CHECK: @__hereB@{{.*}}
  // CHECK: @__subhereB@{{.*}}
  // CHECK: call &bar (addroflabel ptr @__subhereB@{{.*}})
  bar (({__hereB:__subhereB:&&__subhereB;}));
  // CHECK: @__hereC@{{.*}}
  // CHECK: @__subhereC@{{.*}}
  // CHECK: @__fooC@{{.*}}
  // call &bar (addroflabel ptr @__fooC@{{.*}})
  bar (({__hereC:__subhereC:&&__subhereC;__fooC:&&__fooC;}));
  // CHECK: @__hereD@{{.*}}
  // CHECK: @__fooD@{{.*}}
  // call &bar (addroflabel ptr @__fooD@{{.*}})
  bar (({__hereD:&&__hereD;({__fooD:&&__fooD;});}));
  // CHECK: eval (addroflabel ptr @ff{{.*}})
  &&ff;
  printf("it should not be skipped\n");
  ff:
  goto end;
  printf("it should be skipped\n");
  end:;
  return 0;
}
