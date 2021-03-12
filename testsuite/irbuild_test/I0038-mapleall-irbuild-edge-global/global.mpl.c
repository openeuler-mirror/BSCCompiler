#include<stdio.h>
extern char add_i8_u();
extern char add_i8();
extern unsigned char add_u8_u();
extern unsigned char add_u8();

extern short add_i16_u();
extern short add_i16();
extern unsigned short add_u16_u();
extern unsigned short add_u16();

extern int add_i32_u();
extern int add_i32();
extern unsigned int add_u32_u();
extern unsigned int add_u32();

extern long long add_i64_u();
extern long long add_i64();
extern unsigned long long add_u64_u();
extern unsigned long long add_u64();

extern float add_f32_u();
extern float add_f32();

extern double add_f64_u();
extern double add_f64();

int main(){
  char g_i8_u = add_i8_u();
  char g_i8 = add_i8();
  unsigned char g_u8_u = add_u8_u();
  unsigned char g_u8 = add_u8();
  short g_i16_u = add_i16_u();
  short g_i16 = add_i16();
  unsigned short g_u16_u = add_u16_u();
  unsigned short g_u16 = add_i16();
  int g_i32_u = add_i32_u();
  int g_i32 = add_i32();
  unsigned int g_u32_u = add_u32_u();
  unsigned int g_u32 = add_u32();
  
  long long g_i64_u = add_i64_u();
  long long g_i64 = add_i64();
  unsigned long long g_u64_u = add_u64_u();
  unsigned long long g_u64 = add_u64();
  
  // float g_f32_u = add_f32_u();
  // float g_f32 = add_f32();
  
  // double g_f64_u = add_f64_u();
  // double g_f64 = add_f64();
  // i8/u8
  printf("%d\n",g_i8_u);
  printf("%d\n",g_i8);
  if (8 != g_i8) {
    printf("FAIL : g_i8 is not correct\n");
    return 1;
  }
  printf("%d\n",g_u8_u);
  printf("%d\n",g_u8);
  if (8 != g_u8) {
    printf("FAIL : g_i8 is not correct\n");
    return 1;
  }
  
  // i16/u16
  printf("%d\n",g_i16_u);
  printf("%d\n",g_i16);
  if (16 != g_i16) {
    printf("FAIL : g_i16 is not correct\n");
    return 1;
  }
  printf("%d\n",g_u16_u);
  printf("%d\n",g_u16);
  if (16 != g_u16) {
    printf("FAIL : g_u16 is not correct\n");
    return 1;
  }
  
  // i32/u32
  printf("%d\n",g_i32_u);
  printf("%d\n",g_i32);
  if (32 != g_i32) {
    printf("FAIL : g_i32 is not correct\n");
    return 1;
  }
  printf("%d\n",g_u32_u);
  printf("%d\n",g_u32);
  if (32 != g_u32) {
    printf("FAIL : g_u32 is not correct\n");
    return 1;
  }
  
  // i64/u64
  printf("%lld\n",g_i64_u);
  printf("0x%llx\n",g_i64);
  if (0x123456789a != g_i64) {
    printf("FAIL : g_i64 is not correct\n");
    return 1;
  }
  printf("%lld\n",g_u64_u);
  printf("0x%llx\n",g_u64);
  if (0x123456789a != g_u64) {
    printf("FAIL : g_u64 is not correct\n");
    return 1;
  }
  
  // // f32
  // printf("%f\n",g_f32_u);
  // printf("%f\n",g_f32);
  // if (1.2f != g_f32) {
    // printf("FAIL : g_f32 is not correct\n");
    // return 1;
  // }

  // // f64
  // printf("%lf\n",g_f64_u);
  // printf("%lf\n",g_f64);
  // if (2.4 != g_f64) {
    // printf("FAIL : g_f64 is not correct\n");
    // return 1;
  // }
  
  printf("PASS : Scalar global variables initialization test.\n");
  
  return 0;
}
