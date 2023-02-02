#include <stdio.h>
void i32_to_f64() {
  int a = 10;
  double b = a;
  printf("%f\n",  b);
}

void i32_to_f32() {
  int a = 10;
  float b = a;
  printf("%f\n", b);
}

void i64_to_f64() {
  long a = 153;
  double b = a;
  printf("%f\n", b);
}

void i64_to_f32() {
  long a = 153;
  float b = a;
  printf("%f\n", b);
}

void u32_to_f32() {
  unsigned int a = 3;
  float b = a;
  printf("%f\n", b);
}

void u64_to_f64() {
  unsigned long a = 5;
  double b = a;
  printf("%f\n", b);
}

void f32_to_i32() {
  float a = 3.0;
  int b = a;
  printf("%d\n", b);
}

void f64_to_i32() {
  double a = 3.5;
  int b = a;
  printf("%d\n", b);
}

void f32_to_u32() {
  float a = 3.7;
  unsigned int b = a;
  printf("%u\n", b);
}

void f64_to_u32() {
  double a = 3.8;
  unsigned int b = a;
  printf("%u\n", b);
}

void f32_to_u64() {
  float a = 3.8;
  unsigned long b = a;
  printf("%lu\n", b);
}

void f64_to_u64() {
  double a = 3.8;
  unsigned long b = a;
  printf("%lu\n", b);
}

void f32_to_i64() {
  float a = 3.4;
  long b = a;
  printf("%ld\n", b);
}

int main() {
  i32_to_f64();
  i32_to_f32();
  i64_to_f64();
  i64_to_f32();
  u32_to_f32();
  u64_to_f64();
  f32_to_i32();
  f64_to_i32();
  f32_to_u32();
  f64_to_u32();
  f32_to_u64();
  f64_to_u64();
  f32_to_i64();
  return 0;
}

