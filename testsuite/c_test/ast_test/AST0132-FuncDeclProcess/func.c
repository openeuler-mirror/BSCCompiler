// CHECK: [[# FILENUM:]] "{{.*}}/func.c"
#include <stdio.h>
#include <string.h>
// CHECK: LOC [[# FILENUM]] 37
// CHECK: func &abort public used noreturn extern () void
// CHECK: LOC [[# FILENUM]] 38
// CHECK: func &printf public varargs extern (var %format <* u8> restrict, ...) i32
// CHECK: LOC [[# FILENUM]] 39
// CHECK: func &snprintf public used varargs (var %str <* u8>, var %size u64, var %format <* u8>, ...) i32
// CHECK: LOC [[# FILENUM]] 40
// CHECK: func &only_decl public () void
// CHECK: LOC [[# FILENUM]] 42
// CHECK: func &decl_def public () void
// CHECK: LOC [[# FILENUM]] 45
// CHECK: func &only_def public () void
// CHECK: LOC [[# FILENUM]] 49
// CHECK: func &func2 public (var %s <* u8>) i32
// CHECK: LOC [[# FILENUM]] 52
// CHECK: func &func public used () i32
// CHECK: LOC [[# FILENUM]] 56
// CHECK: func &func3 public used (var %n i32) i32
// CHECK: LOC [[# FILENUM]] 65
// CHECK: func &func5 public () void
// CHECK: LOC [[# FILENUM]] 70
// CHECK: func &func4 public used (var %s <* u8> nonnull) void
// CHECK: LOC [[# FILENUM]] 76
// CHECK: func &func6 public (var %n <* i32> nonnull) void
// CHECK: LOC [[# FILENUM]] 81
// CHECK: func &func7 public used noinline (var %n <* i32> nonnull) void
// CHECK: LOC [[# FILENUM]] 88
// CHECK: func &func8 public noinline (var %n <* i32> nonnull) void
// CHECK: LOC [[# FILENUM]] 93
// CHECK: func &func9 public used (var %n <* i32> nonnull) void
// CHECK: LOC [[# FILENUM]] 98
// CHECK: func &func10 public (var %n <* i32> nonnull) void

extern void abort (void);
extern int printf(const char *restrict format, ...);
int snprintf(char *str, size_t size, const char *format, ...);
void only_decl();
void decl_def();
void decl_def() {
  return;
}
void only_def() {
  return;
}
int func();
int func2(char *s) {
  return func();
}
int func() {
  return 0;
}

int func3(int n){
  char s[10];
  char d[5];
  int a = snprintf(s, 10, "123");
  if (a != 3 || strcmp(s, "123") != 0){
    abort();
  }
}
void func4(char *s);
void func5() {
  char *s;
  func4(s);
}
__attribute__((nonnull(1)))
void func4(char *s) {
  int n = 1;
  return;
}

__attribute__((nonnull(1)))
void func6(int *n);

__attribute__((noinline))
void func7(int *n);
__attribute__((nonnull(1)))
void func7(int *n) {
  return;
}

__attribute__((noinline))
void func8(int *n);
__attribute__((nonnull(1)))
void func8(int *n) {
  return;
}

__attribute__((nonnull(1)))
void func9(int *n) {
  return;
}

__attribute__((nonnull(1)))
void func10(int *n) {
  return;
}

int main() {
  int n = 1;
  func7(&n);
  func9(&n);
  return func3(n);
}
