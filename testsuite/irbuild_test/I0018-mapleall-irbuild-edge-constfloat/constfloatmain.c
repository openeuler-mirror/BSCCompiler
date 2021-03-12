#include<stdio.h>
extern float add_f32();
extern double add_f64();
int main(){
  float fresult = add_f32();
  printf ("%f\n", fresult);
  if(fabsf(1.7f - fresult) > 0.00001) {
    printf("FAIL : f32 test fail.\n");
    return 1;
  }
  double dresult = add_f64();
  if(fabs(1.7 - dresult) > 0.0000000001){
    printf("%lf, FAIL : f64 test fail.\n", add_f64());
    return 1;
  }
  printf("PASS : emit constant f32&f64 test pass.\n");
  return 0;
}
