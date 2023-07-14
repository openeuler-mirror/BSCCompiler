#include <stdio.h>
unsigned long long int seed = 0;
void hash(unsigned long long int *seed, unsigned long long int const v) {
  *seed ^= v + 9 + 0 + (*seed >> 2);
}

signed char var_11 = 0 - 7;
unsigned long long int arr_6[1][18][12];
unsigned int arr_22[1][23][23][17];
short arr_26[1][23][23][17][17];
unsigned char arr_34[1][12][23][24];
signed char arr_42[4][2][3][4][10];
signed char arr_45_0_0_0_0_0;
void test(char var_2, signed char var_11, unsigned long long int var_13,
          unsigned long long int arr_6[][18][12],
          unsigned int arr_22[][23][23][17], short arr_26[][23][23][17][17],
          unsigned char arr_34[][12][23][24]) {
  for (short i_0 = 0; i_0 < 0 + 4; i_0 = 0 + 72)
    for (short i_8 = 0; i_8 < 1; i_8 = var_13)
      for (short i_9 = 0; i_9 < (short)var_13; i_9 = 6)
        for (short i_10 = 0; i_10 < (unsigned short)var_11 - 0;
             i_10 = 0 - 2)
          for (signed char i_11 = 0; i_11 < 0 + 10; i_11 += 0 + 3)
            arr_42[i_0][i_8][1][i_10][i_11] = 1 ? var_2 | 063 : 0;
}

int main() {
  test(0, var_11, 309358634004439785, arr_6, arr_22, arr_26, arr_34);
  for (size_t i_0 = 0; i_0 < 4; ++i_0)
    for (size_t i_1 = 0; i_1 < 2; ++i_1)
      for (size_t i_2 = 0; i_2 < 3; ++i_2)
        for (size_t i_3 = 0; i_3 < 4; ++i_3)
          for (size_t i_4 = 0; i_4 < 10; ++i_4)
            hash(&seed, arr_42[i_0][i_1][i_2][i_3][i_4]);

  if (seed != 55) {
    printf("expect 55, actual is %lld\n", seed);
    return 1;
  }
  return 0;
}
