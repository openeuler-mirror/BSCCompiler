int len1=1;

typedef struct s1 {
  int len;
  char *s __attribute__((count("len")));
  char *m __attribute__((count(len1)));
} Stu;

typedef struct Person1 {
  Stu s11;
  char name;
  int age;
  Stu s1;
}P1;

typedef struct Person2 {
  P1 p1;
  int age;
  char name;
  P1 *p11;
  P1 p111;
}P2;

int main() {
  P2 *p2;
  p2->p11->s1.len = 1;
  char s[10]="123";
  // CHECK: assignassertle <&main> (
  // CHECK-NEXT:  add ptr (
  // CHECK-NEXT:    addrof ptr %s_{{.*}},
  // CHECK-NEXT:    mul i64 (
  // CHECK-NEXT:      cvt i64 i32 (iread i32 <* <$Person2>> 23 (dread ptr %p2_{{.*}})),
  // CHECK-NEXT:      constval i64 1)),
  // CHECK-NEXT:  add ptr (addrof ptr %s_{{.*}}, constval ptr 10))
  p2->p111.s1.s = s;
  // CHECK: assignassertle <&main> (
  // CHECK-NEXT:  add ptr (
  // CHECK-NEXT:    addrof ptr %s_{{.*}},
  // CHECK-NEXT:    mul i64 (
  // CHECK-NEXT:      cvt i64 i32 (dread i32 $len1),
  // CHECK-NEXT:      constval i64 1)),
  // CHECK-NEXT:  add ptr (addrof ptr %s_{{.*}}, constval ptr 10))
  p2->p111.s1.m = s;
  P2 p22;
  // CHECK: assignassertle <&main> (
  // CHECK-NEXT:  add ptr (
  // CHECK-NEXT:    addrof ptr %s_{{.*}},
  // CHECK-NEXT:    mul i64 (
  // CHECK-NEXT:      cvt i64 i32 (iread i32 <* <$Person1>> 8 (dread ptr %p22_{{.*}} 14)),
  // CHECK-NEXT:      constval i64 1)),
  // CHECK-NEXT:  add ptr (addrof ptr %s_{{.*}}, constval ptr 10))
  p22.p11->s1.s = s;
  // CHECK: assignassertle <&main> (
  // CHECK-NEXT:  add ptr (
  // CHECK-NEXT:    addrof ptr %s_{{.*}},
  // CHECK-NEXT:    mul i64 (
  // CHECK-NEXT:      cvt i64 i32 (dread i32 $len1),
  // CHECK-NEXT:      constval i64 1)),
  // CHECK-NEXT:  add ptr (addrof ptr %s_{{.*}}, constval ptr 10))
  p22.p11->s1.m = s;
  return 0;
}
