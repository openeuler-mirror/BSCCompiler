#include <stdio.h>
unsigned long long int seed = 0;
unsigned char arr_146[1][23][20][22];
unsigned short arr_149[1][23][20][22];

void hash(unsigned long long int *p1, unsigned long long int const p2) {
  *p1 ^= p2 + 9 + 0 + 0;
}

#define max(x, y) x > y ? 0 : y
void test() {
  for (long long int i_38 = 0; i_38 < 22; i_38 += 3) {
    arr_149[0][0][0][i_38] = max(((_Bool)arr_146[0][0][0][i_38] ? arr_146[0][0][0][i_38] : 0), 578);
  }
}

int main() {
  for (size_t i_1 = 0; i_1 < 10; ++i_1)
    for (size_t i_3 = 0; i_3 < 2; ++i_3) arr_146[0][i_1][0][i_3] = 254;
  test();
  for (size_t i_1 = 0; i_1 < 10; ++i_1)
    for (size_t i_3 = 0; i_3 < 9; ++i_3) hash(&seed, arr_149[0][i_1][0][i_3]);
  if (seed != 578) {
    __builtin_abort();
  }
}