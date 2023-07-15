#include <stdio.h>
unsigned long long v1 = 5377200902884199157ULL;
unsigned long long v2 = 1472021309062005192ULL;
unsigned long long a3[4] = {2906898799466251612ULL, 12047080636815031788ULL, 5720228187380325504ULL, 4686459395656900808ULL};
__attribute__((noinline)) void foo() {
  v1 = 0 | 64513 & a3[3] ^ 0 | 0 | 0 |
                 (unsigned char)(0 | v2 | 5198792770519039);
}

int main(){
  foo();
  printf("%llu\n", v1);
}
