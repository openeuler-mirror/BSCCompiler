#include <stdio.h>

unsigned long long int seed = 0;
void hash(unsigned long long int *seed, unsigned long long int const v) {
  *seed ^= v + 0x9e3779b9 + ((*seed) << 6) + ((*seed) >> 2);
}

unsigned int var_0 = 241429283U;
long long int var_1 = 5837214733273594058LL;
_Bool var_2 = (_Bool)1;
signed char var_3 = (signed char)42;
unsigned int var_4 = 2135023861U;
unsigned long long int var_5 = 991743411189159239ULL;
int var_6 = 869598879;
unsigned short var_7 = (unsigned short)58971;
unsigned long long int var_8 = 11399711916651176405ULL;
long long int var_9 = -3965251633771929298LL;
unsigned long long int var_10 = 2904209907058274201ULL;
unsigned int var_11 = 3479243816U;
short var_12 = (short)13292;
short var_13 = (short)-17655;
signed char var_14 = (signed char)-27;
int var_15 = -219605940;
unsigned char var_16 = (unsigned char)73;
_Bool var_17 = (_Bool)0;
short var_18 = (short)-14699;
int arr_0[11];
unsigned short arr_1[11];
_Bool arr_3[11][21];
_Bool arr_13[11][17][21];
unsigned char arr_2[11];
signed char arr_8[11][21][24];
signed char arr_9[11];
long long int arr_14[11][17][21];
signed char arr_15[11];
int arr_16[11];

void init() {
  for (size_t i_0 = 0; i_0 < 11; ++i_0) 
    arr_0[i_0] = 2086668070;
  for (size_t i_0 = 0; i_0 < 11; ++i_0) 
    arr_1[i_0] = (unsigned short)33276;
  for (size_t i_0 = 0; i_0 < 11; ++i_0) 
    for (size_t i_1 = 0; i_1 < 21; ++i_1) 
      arr_3[i_0][i_1] = (_Bool)1;
  for (size_t i_0 = 0; i_0 < 11; ++i_0) 
    for (size_t i_1 = 0; i_1 < 17; ++i_1) 
      for (size_t i_2 = 0; i_2 < 21; ++i_2) 
        arr_13[i_0][i_1][i_2] = (_Bool)0;
  for (size_t i_0 = 0; i_0 < 11; ++i_0) 
    arr_2[i_0] = (unsigned char)253;
  for (size_t i_0 = 0; i_0 < 11; ++i_0) 
    for (size_t i_1 = 0; i_1 < 21; ++i_1) 
      for (size_t i_2 = 0; i_2 < 24; ++i_2) 
        arr_8[i_0][i_1][i_2] = (signed char)89;
  for (size_t i_0 = 0; i_0 < 11; ++i_0) 
    arr_9[i_0] = (signed char)-91;
  for (size_t i_0 = 0; i_0 < 11; ++i_0) 
    for (size_t i_1 = 0; i_1 < 17; ++i_1) 
      for (size_t i_2 = 0; i_2 < 21; ++i_2) 
        arr_14[i_0][i_1][i_2] = 7060654226781065427LL;
  for (size_t i_0 = 0; i_0 < 11; ++i_0) 
    arr_15[i_0] = (signed char)-55;
  for (size_t i_0 = 0; i_0 < 11; ++i_0) 
    arr_16[i_0] = -1100771191;
}

void checksum() {
  hash(&seed, var_10);
  hash(&seed, var_11);
  hash(&seed, var_12);
  hash(&seed, var_13);
  hash(&seed, var_14);
  hash(&seed, var_15);
  hash(&seed, var_16);
  hash(&seed, var_17);
  hash(&seed, var_18);
  for (size_t i_0 = 0; i_0 < 11; ++i_0) 
    hash(&seed, arr_2[i_0]);
  for (size_t i_0 = 0; i_0 < 11; ++i_0) 
    for (size_t i_1 = 0; i_1 < 21; ++i_1) 
      for (size_t i_2 = 0; i_2 < 24; ++i_2) 
        hash(&seed, arr_8[i_0][i_1][i_2]);
  for (size_t i_0 = 0; i_0 < 11; ++i_0) 
    hash(&seed, arr_9[i_0]);
  for (size_t i_0 = 0; i_0 < 11; ++i_0) 
    for (size_t i_1 = 0; i_1 < 17; ++i_1) 
      for (size_t i_2 = 0; i_2 < 21; ++i_2) 
        hash(&seed, arr_14[i_0][i_1][i_2]);
  for (size_t i_0 = 0; i_0 < 11; ++i_0) 
    hash(&seed, arr_15[i_0]);
  for (size_t i_0 = 0; i_0 < 11; ++i_0) {
    hash(&seed, arr_16[i_0]);
  }
}

void test(unsigned int var_0, long long int var_1, _Bool var_2, signed char var_3, unsigned int var_4,
          unsigned long long int var_5, int var_6, unsigned short var_7, unsigned long long int var_8,
          long long int var_9, int arr_0[11], unsigned short arr_1[11], _Bool arr_3[11][21], _Bool arr_13[11][17][21]);

int main() {
  init();
  test(var_0, var_1, var_2, var_3, var_4, var_5, var_6, var_7, var_8, var_9, arr_0 , arr_1 , arr_3 , arr_13);
  checksum();
  printf("%llu\n", seed);
}
