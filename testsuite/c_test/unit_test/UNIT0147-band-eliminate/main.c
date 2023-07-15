#include "stdio.h"

typedef struct  {
    unsigned long int member_5_0;
    unsigned long long int member_5_1;
    unsigned long long int member_5_2;
} tf_3_struct_5;

int a[6];
int b = 3;

tf_3_struct_5 tf_3_array_6[1] = {{5566634620210400175UL, 3633139089054659200ULL, 16380977235067372784ULL}};
int tf_3_var_334 = -2007184200;

__attribute__((noinline)) void tf_2_foo() {
  if ((0 < b) << (0 <= 0) & 0 - 90 - 1)
    a[3] = 2;
}


__attribute__((noinline)) void tf_3_foo() {
  tf_3_var_334 = (0 != tf_3_array_6[0].member_5_0) << 8 & 0x1F40A465;
}

int main(){
  tf_2_foo();
  tf_3_foo();
  printf("%d\n",a[3]);
  printf("%d\n",tf_3_var_334);
}
