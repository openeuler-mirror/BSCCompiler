#ifndef likely
#define likely(x) __builtin_expect((x), 1)
#endif

#ifndef unlikely
#define unlikely(x) __builtin_expect((x), 0)
#endif

// CHECK: [[# FILENUM:]] "{{.*}}/builtinexpect.c"

// each pair of following functions have equivalent __builtin_expect intrinsicop in mpl
void and_un(int a, int b) {
  // CHECK: func &and_un
  // CHECK: brfalse @shortCircuit_{{.*}}
  // CHECK-NEXT: intrinsicop i64 C___builtin_expect
  // CHECK: brfalse @shortCircuit_{{.*}}
  // CHECK-NEXT:intrinsicop i64 C___builtin_expect
  // CHECK: return ()
  if (unlikely(a != 0) && unlikely(b != 0)) {}
}
void un_and(int a, int b) {
  // CHECK: func &un_and
  // CHECK: brfalse @shortCircuit_{{.*}}
  // CHECK-NEXT: intrinsicop i64 C___builtin_expect
  // CHECK: brfalse @shortCircuit_{{.*}}
  // CHECK-NEXT:intrinsicop i64 C___builtin_expect
  // CHECK: return ()
  if (unlikely(a != 0 && b != 0)) {}
}


void or_un(int a, int b) {
  // CHECK: func &or_un
  // CHECK: brtrue @shortCircuit_{{.*}}
  // CHECK-NEXT: intrinsicop i64 C___builtin_expect
  // CHECK: brtrue @shortCircuit_{{.*}}
  // CHECK-NEXT:intrinsicop i64 C___builtin_expect
  // CHECK: return ()
  if (unlikely(a != 0) || unlikely(b != 0)) {}
}
void un_or(int a, int b) {
  // CHECK: func &un_or
  // CHECK: brtrue @shortCircuit_{{.*}}
  // CHECK-NEXT: intrinsicop i64 C___builtin_expect
  // CHECK: brtrue @shortCircuit_{{.*}}
  // CHECK-NEXT:intrinsicop i64 C___builtin_expect
  // CHECK: return ()
  if (unlikely(a != 0 || b != 0)) {}
}


void and_likely(int a, int b) {
  // CHECK: func &and_likely
  // CHECK: brfalse @shortCircuit_{{.*}}
  // CHECK-NEXT: intrinsicop i64 C___builtin_expect
  // CHECK: brfalse @shortCircuit_{{.*}}
  // CHECK-NEXT:intrinsicop i64 C___builtin_expect
  // CHECK:return ()
  if (likely(a != 0) && likely(b != 0)) {}
}
void likely_and(int a, int b) {
  // CHECK: func &likely_and
  // CHECK: brfalse @shortCircuit_{{.*}}
  // CHECK-NEXT: intrinsicop i64 C___builtin_expect
  // CHECK: brfalse @shortCircuit_{{.*}}
  // CHECK-NEXT:intrinsicop i64 C___builtin_expect
  // CHECK: return ()
  if (likely(a != 0 && b != 0)) {}
}


void or_likely(int a, int b) {
  // CHECK: func &or_likely
  // CHECK: brtrue @shortCircuit_{{.*}}
  // CHECK-NEXT: intrinsicop i64 C___builtin_expect
  // CHECK: brtrue @shortCircuit_{{.*}}
  // CHECK-NEXT:intrinsicop i64 C___builtin_expect
  // CHECK: return ()
  if (likely(a != 0) || likely(b != 0)) {}
}
void likely_or(int a, int b) {
  // CHECK: func &likely_or
  // CHECK: brtrue @shortCircuit_{{.*}}
  // CHECK-NEXT: intrinsicop i64 C___builtin_expect
  // CHECK: brtrue @shortCircuit_{{.*}}
  // CHECK-NEXT:intrinsicop i64 C___builtin_expect
  // CHECK return ()
  if (likely(a != 0 || b != 0)) {}
}


void and_or_likely(int a, int b, int c) {
  // CHECK: func &and_or_likely
  // CHECK: brfalse @shortCircuit_{{.*}}
  // CHECK-NEXT: intrinsicop i64 C___builtin_expect
  // CHECK: brtrue @shortCircuit_{{.*}}
  // CHECK-NEXT:intrinsicop i64 C___builtin_expect
  // CHECK: brtrue @shortCircuit_{{.*}}
  // CHECK-NEXT:intrinsicop i64 C___builtin_expect
  // CHECK: return ()
  if (likely(a!= 0) && (likely(b != 0) || likely(c!=0))) {}
}
void likely_and_or(int a, int b , int c) {
  // CHECK: func &likely_and_or
  // CHECK: brfalse @shortCircuit_{{.*}}
  // CHECK-NEXT: intrinsicop i64 C___builtin_expect
  // CHECK: brtrue @shortCircuit_{{.*}}
  // CHECK-NEXT:intrinsicop i64 C___builtin_expect
  // CHECK: brtrue @shortCircuit_{{.*}}
  // CHECK-NEXT:intrinsicop i64 C___builtin_expect
  // CHECK: return ()
  if (likely(a!= 0 && (b != 0 || c != 0))) {}
}


void or_and_likely(int a, int b, int c) {
  // CHECK: func &or_and_likely
  // CHECK: brtrue @shortCircuit_{{.*}}
  // CHECK-NEXT: intrinsicop i64 C___builtin_expect
  // CHECK: brfalse @shortCircuit_{{.*}}
  // CHECK-NEXT:intrinsicop i64 C___builtin_expect
  // CHECK: brfalse @shortCircuit_{{.*}}
  // CHECK-NEXT:intrinsicop i64 C___builtin_expect
  // CHECK: return ()
  if (likely(a!= 0) || (likely(b != 0) && likely(c!=0))) {}
}
void likely_or_and(int a, int b , int c) {
  // CHECK: func &likely_or_and
  // CHECK: brtrue @shortCircuit_{{.*}}
  // CHECK-NEXT: intrinsicop i64 C___builtin_expect
  // CHECK: brfalse @shortCircuit_{{.*}}
  // CHECK-NEXT:intrinsicop i64 C___builtin_expect
  // CHECK: brfalse @shortCircuit_{{.*}}
  // CHECK-NEXT:intrinsicop i64 C___builtin_expect
  // CHECK:return ()
  if (likely(a!= 0 || (b != 0 && c != 0))) {}
}


void un_and_likely_and(int a, int b, int c) {
  // CHECK: func &un_and_likely_and
  // CHECK: brfalse @shortCircuit_{{.*}}
  // CHECK-NEXT: intrinsicop i64 C___builtin_expect
  // CHECK: brfalse @shortCircuit_{{.*}}
  // CHECK-NEXT:intrinsicop i64 C___builtin_expect
  // CHECK: brfalse @shortCircuit_{{.*}}
  // CHECK-NEXT:intrinsicop i64 C___builtin_expect
  // return ()
  if(unlikely(a != 0) && likely(b != 0 && c != 0)) {}
}
void un_and_and_likely(int a, int b, int c) {
  // CHECK: func &un_and_and_likely
  // CHECK: brfalse @shortCircuit_{{.*}}
  // CHECK-NEXT: intrinsicop i64 C___builtin_expect
  // CHECK: brfalse @shortCircuit_{{.*}}
  // CHECK-NEXT:intrinsicop i64 C___builtin_expect
  // CHECK: brfalse @shortCircuit_{{.*}}
  // CHECK-NEXT:intrinsicop i64 C___builtin_expect
  // return ()
  if(unlikely(a != 0) && likely(b != 0) && likely(c != 0)) {}
}


void un_and_likely_or(int a, int b, int c) {
  // CHECK: func &un_and_likely_or
  // CHECK: brfalse @shortCircuit_{{.*}}
  // CHECK-NEXT: intrinsicop i64 C___builtin_expect
  // CHECK: brtrue @shortCircuit_{{.*}}
  // CHECK-NEXT:intrinsicop i64 C___builtin_expect
  // CHECK: brtrue @shortCircuit_{{.*}}
  // CHECK-NEXT:intrinsicop i64 C___builtin_expect
  // return ()
  if(unlikely(a != 0) && likely(b != 0 || c != 0)) {}
}
void un_and_or_likely(int a, int b, int c) {
  // CHECK: func &un_and_or_likely
  // CHECK: brfalse @shortCircuit_{{.*}}
  // CHECK-NEXT: intrinsicop i64 C___builtin_expect
  // CHECK: brtrue @shortCircuit_{{.*}}
  // CHECK-NEXT:intrinsicop i64 C___builtin_expect
  // CHECK: brtrue @shortCircuit_{{.*}}
  // CHECK-NEXT:intrinsicop i64 C___builtin_expect
  // return ()
  if(unlikely(a != 0) && (likely(b != 0) || likely(c != 0))) {}
}


void un_or_liklely_or(int a, int b, int c) {
  // CHECK: func &un_or_liklely_or
  // CHECK: brtrue @shortCircuit_{{.*}}
  // CHECK-NEXT: intrinsicop i64 C___builtin_expect
  // CHECK: brtrue @shortCircuit_{{.*}}
  // CHECK-NEXT:intrinsicop i64 C___builtin_expect
  // CHECK: brtrue @shortCircuit_{{.*}}
  // CHECK-NEXT:intrinsicop i64 C___builtin_expect
  // return ()
  if(unlikely(a != 0) || likely(b != 0 || c != 0)) {}
}
void un_or_or_likely(int a, int b, int c) {
  // CHECK: func &un_or_or_likely
  // CHECK: brtrue @shortCircuit_{{.*}}
  // CHECK-NEXT: intrinsicop i64 C___builtin_expect
  // CHECK: brtrue @shortCircuit_{{.*}}
  // CHECK-NEXT:intrinsicop i64 C___builtin_expect
  // CHECK: brtrue @shortCircuit_{{.*}}
  // CHECK-NEXT:intrinsicop i64 C___builtin_expect
  // return ()
  if(unlikely(a != 0) || likely(b != 0) || likely(c != 0)) {}
}


void un_or_likely_and(int a, int b, int c) {
  // CHECK: func &un_or_likely_and
  // CHECK: brtrue @shortCircuit_{{.*}}
  // CHECK-NEXT: intrinsicop i64 C___builtin_expect
  // CHECK: false @shortCircuit_{{.*}}
  // CHECK-NEXT:intrinsicop i64 C___builtin_expect
  // CHECK: false @shortCircuit_{{.*}}
  // CHECK-NEXT:intrinsicop i64 C___builtin_expect
  // return ()
  if(unlikely(a != 0) || likely(b != 0 && c != 0)) {}
}
void un_or_and_likely(int a, int b, int c) {
  // CHECK: func &un_or_and_likely
  // CHECK: brtrue @shortCircuit_{{.*}}
  // CHECK-NEXT: intrinsicop i64 C___builtin_expect
  // CHECK: false @shortCircuit_{{.*}}
  // CHECK-NEXT:intrinsicop i64 C___builtin_expect
  // CHECK: false @shortCircuit_{{.*}}
  // CHECK-NEXT:intrinsicop i64 C___builtin_expect
  // return ()
  if(unlikely(a != 0) || (likely(b != 0) && likely(c != 0))) {}
}

int main() {
  return 0;
}
