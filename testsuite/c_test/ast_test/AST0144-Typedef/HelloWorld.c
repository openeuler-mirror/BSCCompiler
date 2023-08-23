// CHECK: [[# FILENUM:]] "{{.*}}/HelloWorld.c"

// CHECK: type $A <struct typedef {
typedef struct {
  int a;
  int b;
} A;

// CHECK: type $B <struct typedef align(16) {
typedef __attribute__((aligned(16)))
struct {
  unsigned long long w[3];
} B;

// CHECK: type $Bar <struct {
typedef struct Bar {
  int a;
  char charfield[129];
  int b;
} NewBar __attribute__((aligned(128)));

typedef int __attribute__((aligned(64))) myint;

// CHECK: type $Foo <struct {
struct Foo {
  struct Bar a;
  NewBar b;
  // CHECK:   @c i32 type_align(64) align(128)}>
  myint c __attribute__((aligned(128)));
};

// CHECK: type $C <union typedef pack(2) {
#pragma pack(2)
typedef union {
  int a;
  A a_a;
} C;

// CHECK: type $NewBar <struct typedef type_alias($Bar) align(128) {
// CHECK: type $NewNewBar <struct typedef type_alias($NewBar, $Bar) align(64) {
typedef NewBar  __attribute__((aligned(64))) NewNewBar;
// CHECK: type $NewNewNewBar <struct typedef type_alias($NewNewBar, $NewBar, $Bar) align(256) {
typedef NewNewBar  __attribute__((aligned(256))) NewNewNewBar;

int main() {
  return 0;
}