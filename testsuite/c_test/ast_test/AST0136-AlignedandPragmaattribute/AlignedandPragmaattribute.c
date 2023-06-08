// CHECK: [[# FILENUM:]] "{{.*}}/AlignedandPragmaattribute.c"
struct A {
  // CHECK: type $A <struct {
  // CHECK:   @a u8
  // CHECK-NEXT:   @b i64 packed asmattr align(4)
  char a;
  __attribute__((aligned(4), packed)) long long b;
};

struct __attribute__((aligned(4), packed)) B {
  // CHECK: type $B <struct packed align(4) {
  // CHECK:   @a u8
  // CHECK-NEXT:   @b i64
  char a;
  long long b;
};

#pragma pack(2)
struct __attribute__ ((aligned(8))) C {
  // CHECK: type $C <struct align(8) pack(2) {
  // CHECK:   @a u8
  // CHECK-NEXT:   @b i32 packed asmattr align(4)
  // CHECK-NEXT:   @c f64
  char a;
  __attribute__((aligned(4), packed)) int b;
  double c;
};

#pragma pack(4)
struct D {
  // CHECK: type $D <struct pack(4) {
  // CHECK:   @a :1 u8
  // CHECK-NEXT:   @b :1 i32 asmattr align(8)
  char a : 1;
  __attribute__ ((aligned(8))) int b : 1;
};

#pragma pack(1)
struct E {
  // CHECK: type $E <struct pack(1) {
  // CHECK:   @a :1 u8
  // CHECK-NEXT:   @b :1 i32 asmattr align(2)
  char a : 1;
  __attribute__ ((aligned(2))) int b : 1;
};

int main() {
  return 0;
}
