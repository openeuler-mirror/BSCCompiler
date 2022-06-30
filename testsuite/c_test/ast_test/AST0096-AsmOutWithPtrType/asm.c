// CHECK: [[# FILENUM:]] "{{.*}}/asm.c"
struct subA {
  int b;
};
struct A {
  long a;
  struct subA f;
};

static inline void foo1(struct A *x)
{
 // CHECK: var %asm_out_{{.*}} i64
 // CHECK: {{.*}} dassign %asm_out_{{.*}} 0
 // CHECK: iassign <* <$A{{.*}}>> 1 (dread ptr %x, dread i64 %asm_out_{{.*}})
  __asm__ __volatile__("" : "+m"(x->a) : "r"(x) : "memory", "cc");
}

static inline void foo2(struct A *x)
{
 // CHECK: var %asm_out_{{.*}} <$subA{{.*}}>
 // CHECK: {{.*}} dassign %asm_out_{{.*}} 0
 // CHECK: iassign <* <$A{{.*}}>> 2 (dread ptr %x, dread agg %asm_out_{{.*}})
  __asm__ __volatile__("" : "+m"(x->f) : "r"(x) : "memory", "cc");
}

static inline void foo3(struct A *x)
{
 // CHECK: var %asm_out_{{.*}} i64
 // CHECK: {{.*}} dassign %asm_out_{{.*}} 0
 // CHECK: iassign <* <$A{{.*}}>> 1 (dread ptr %x, dread i64 %asm_out_{{.*}})
  __asm__ __volatile__("" : "+r"(x->a) : "r"(x) : "memory", "cc");
}

static inline void foo4(struct A *x)
{
 // CHECK: var %asm_out_{{.*}} <$subA{{.*}}>
 // CHECK: {{.*}} dassign %asm_out_{{.*}} 0
 // CHECK: iassign <* <$A{{.*}}>> 2 (dread ptr %x, dread agg %asm_out_{{.*}})
  __asm__ __volatile__("" : "+r"(x->f) : "r"(x) : "memory", "cc");
}

int main() {
  return 0;
}
