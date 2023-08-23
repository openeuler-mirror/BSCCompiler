#include <stdio.h>

signed char var_4 = (signed char)19;
long long int var_12 = -6387642471029973383LL;
unsigned long long int var_14 = 1142716087604872716ULL;
signed char var_16 = (signed char)81;
unsigned int var_26 = 3123253222U;
unsigned int arr_33[18][23];
unsigned int arr_34[18][23];
unsigned long long int arr_330[15];

void init() {
  for (size_t i_0 = 0; i_0 < 18; ++i_0)
    for (size_t i_1 = 0; i_1 < 23; ++i_1)
      arr_33[i_0][i_1] = 1995976896U;
  for (size_t i_0 = 0; i_0 < 18; ++i_0)
    for (size_t i_1 = 0; i_1 < 23; ++i_1)
      arr_34[i_0][i_1] = 730469611U;
}

void test(signed char var_4,
          long long int var_12,
          unsigned long long int var_14,
          signed char var_16,
          unsigned int arr_33[18][23],
          unsigned int arr_34[18][23]);

int main() {
  init();
  test(var_4, var_12, var_14, var_16, arr_33, arr_34);
  printf("%llu\n", var_26);
}
