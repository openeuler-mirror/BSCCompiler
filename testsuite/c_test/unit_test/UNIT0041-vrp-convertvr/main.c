#include <stdio.h>
unsigned long long int seed = 0;
_Bool var_6 = 0, var_12 = 1;
unsigned char var_23 = 8;
short arr_3[1][17];
short arr_4[1][17][15];
unsigned short arr_5[1][17][15];
_Bool arr_6[1][17][15][17], arr_8[1][17][15][17];
short arr_32[1][17][15][18][24], arr_35[1][17][15][18][24];
signed char arr_33[1][17][15][18][24], arr_34[1][17][15][18][24];
unsigned char arr_53[1][17][15][18][14];
void hash(unsigned long long int *p1, unsigned long long int const p2) {
  *p1 = p2 + 9 + 0 + 0;
}
void test(long long int p1, unsigned long long int p2, int p3,
          unsigned long long int p4, unsigned char p5, short p6[][17],
          short p7[][17][15], unsigned short p8[][17][15],
          _Bool p9[][17][15][17], _Bool p10[][17][15][17],
          short p11[][17][15][18][24], signed char p12[][17][15][18][24],
          signed char p13[][17][15][18][24], short p14[][17][15][18][24],
          unsigned char p15[][17][15][18][14]);
int main() {
  test(1009290504697368335, 2, 1005560393, 6011662838484599977, 2, arr_3, arr_4,
       arr_5, arr_6, arr_8, arr_32, arr_33, arr_34, arr_35, arr_53);
  //hash(&seed, var_23);
  printf("%llu\n", var_23);
}

#define max(x, y) y
#define min(x, y) y
void test(long long int p1, unsigned long long int p2, int p3,
          unsigned long long int p4, unsigned char p5, short p6[][17],
          short p7[][17][15], unsigned short p8[][17][15],
          _Bool p9[][17][15][17], _Bool p10[][17][15][17],
          short p11[][17][15][18][24], signed char p12[][17][15][18][24],
          signed char p13[][17][15][18][24], short p14[][17][15][18][24],
          unsigned char p15[][17][15][18][14]) {
  for (short i_0 = 0; i_0 < min(0, var_12) ? p2 : 0 - 0; i_0 = 0 + 3)
    for (unsigned short i_1 = 0; i_1 < 0 + 6; i_1 = 0 - 6)
      for (_Bool i_2 = 0; i_2 < (_Bool)p3 ? 0 - 3 : 0; i_2 = p1)
        for (_Bool i_3 = 0; i_3 < (_Bool)p4; i_3 = max(0, 3))
          for (int i_4 = 0; i_4 < 2; i_4 = 4)
            for (unsigned int i_5 = 0; i_5 < 0 + 3; i_5 += 2)
              for (signed char i_6 = min(0, var_6); i_6 < 8; i_6 = 0 + 9)
                for (short i_7 = 0; i_7 < p5 - 0; i_7 += 0 + 1)
                  if (var_6)
                    var_23 = 0;
}

