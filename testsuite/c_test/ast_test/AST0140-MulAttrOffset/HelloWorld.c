#include<stdint.h>
// CHECK: [[# FILENUM:]] "{{.*}}/HelloWorld.c"
struct a {
  int8_t b;
} __attribute__((aligned, aligned(4)));
struct {
  struct a c;
  struct a d;
} e;
// CHECK: LOC {{.*}} 12 9
// CHECK-NEXT: var $f <* i8> used = addrof ptr $e (4)
int8_t *f = &e.d.b;

struct A {
  int64_t b;
} __attribute__((aligned, aligned));
struct C {
  struct A d;
};
struct {
  struct C e;
} h;
// CHECK: LOC {{.*}} 25 10
// CHECK-NEXT: var $g <* i64> used = addrof ptr $h
int64_t *g = &h.e.d.b;

struct S0 {
  unsigned f4;
} __attribute__((aligned, warn_if_not_aligned0, deprecated, unused))
__attribute__((aligned(1), deprecated, unused, transparent_union));
struct {
  struct S0 f1;
  unsigned long f7;
} __attribute__(()) a, b;
// CHECK: LOC {{.*}} 37 17
// CHECK-NEXT: var $c <* u64> = addrof ptr $b (8)
unsigned long * c = &b.f7;

int32_t funcA() {
  if (*f = 5)
    return 0;
  if (*g = 0)
    return 0;
}
int main() {
  funcA();
}
