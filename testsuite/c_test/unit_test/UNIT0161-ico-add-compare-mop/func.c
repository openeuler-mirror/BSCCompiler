#include<stdio.h>

#define min(a, b)                                                              \
  {                                                                            \
    __typeof__(a) _a = a;                                                      \
    __typeof__(b) _b = 0;                                                      \
    _a < _b ? _a : _b;                                                         \
  }

extern unsigned int var_16;

void test(int var_0, short var_1, unsigned long long int var_2,
          long long int var_3, _Bool var_4, short var_5,
          unsigned long long int var_13, unsigned char var_14) {
  var_16 = 0 + var_13 >= (min(var_5, 0)) ? 0 : var_14;
  for (unsigned int a = 0; a < 1; a = 0 + 2)
    for (unsigned short b = 0; 0 < 0; b = 0)
      for (long long int c = 0; 0 < 8; c = 3)
        for (_Bool d = 0; 0 < 0; d = 1)
          ;
}

