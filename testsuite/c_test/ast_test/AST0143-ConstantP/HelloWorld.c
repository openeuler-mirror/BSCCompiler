#include <stdio.h>
#include <complex.h>

enum {
  RED = 0,
  BLUE = 1,
  GREEN = 2,
};

int main() {
  int n;
  // CHECK: if (constval u1 1)
  if(__builtin_constant_p("abc")) {}
  // CHECK: dassign %intrinsicop_var_{{.*}} 0 (intrinsicop i32 C_constant_p (dread i32 %n_{{.*}}))
  // CHECK: if (ne u1 i32 (dread i32 %intrinsicop_var_{{.*}}, constval i32 0))
  if(__builtin_constant_p(n)) {}
  // CHECK: if (constval u1 1)
  if(__builtin_constant_p(1)) {}
  // CHECK: if (constval u1 1)
  if(__builtin_constant_p(1 + I)) {}
  // CHECK: if (constval u1 1)
  if(__builtin_constant_p(RED)) {}
  // CHECK: if (constval u1 1)
  if(__builtin_constant_p(1.0)) {}
  // CHECK: dassign %intrinsicop_var_{{.*}} 0 (intrinsicop i32 C_constant_p (dread i32 %n_{{.*}}))
  // CHECK: brtrue @shortCircuit_{{.*}} (eq u1 i32 (dread i32 %intrinsicop_var_{{.*}}, constval i32 0))
  if(!__builtin_constant_p(n) || n < 0) {}

  int i=0;
  // CHECK: if (constval u1 0)
  if(__builtin_constant_p(i++)) {}
  // CHECK: dassign %intrinsicop_var_{{.*}} 0 (intrinsicop i32 C_constant_p (dread i32 %i_{{.*}}))
  if(1.0 * __builtin_constant_p(i)) {}
  // CHECK: mul f64 (
  // CHECK:   constval f64 1
  // CHECK:   cvt f64 i32 (constval i32 1))
  if(1.0 * __builtin_constant_p(1)) {}
  return 0;
}
