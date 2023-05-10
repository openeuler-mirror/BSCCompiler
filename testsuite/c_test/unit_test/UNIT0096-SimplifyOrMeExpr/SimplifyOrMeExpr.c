#include <stdint.h>
#include <stdio.h>

__attribute__((noinline)) int8_t foo_8() {
  return 0xF;
}
void func_8() {
  int8_t a = foo_8();
  int8_t b = 4;
  int8_t c = -16;
  c = (a ^ b) | c;
  printf("%d\n", c);
}

__attribute__((noinline)) int16_t foo_16() {
  return 0xF;
}
void func_16() {
  int16_t a = foo_16();
  int16_t b = 4;
  int16_t c = -16;
  c = (a ^ b) | c;
  printf("%d\n", c);
}

__attribute__((noinline)) int32_t foo_32() {
  return 0xF;
}
void func_32() {
  int32_t a = foo_32();
  int32_t b = 4;
  int32_t c = -16;
  c = (a ^ b) | c;
  printf("%d\n", c);
}

__attribute__((noinline)) int64_t foo_64() {
  return 0xF;
}
void func_64() {
  int64_t a = foo_64();
  int64_t b = 4;
  int64_t c = -16;
  c = (a ^ b) | c;
  printf("%ld\n", c);
}

int main() {
  func_8();
  func_16();
  func_32();
  func_64();
  return 0;
}
