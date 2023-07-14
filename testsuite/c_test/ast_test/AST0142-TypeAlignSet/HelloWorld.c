// CHECK: [[# FILENUM:]] "{{.*}}/HelloWorld.c"

#include<stdint.h>
struct S1 {
  uint32_t f0;
} __attribute__((aligned, warn_if_not_aligned(0)))
__attribute__((aligned(2), warn_if_not_aligned(0)));

// CHECK: LOC {{.*}} 11 11
// CHECK-NEXT: var $g_a <[7][5] <$S1>> used
struct S1 g_a[7][5];
// CHECK: LOC {{.*}} 14 21
// CHECK-NEXT: var $g_b <* <$S1>> volatile = addrof ptr $g_a (128)
struct S1 *volatile g_b = &g_a[6][2];

int main() {
  return 0;
}
