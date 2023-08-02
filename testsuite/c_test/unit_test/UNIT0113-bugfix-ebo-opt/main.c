#include <stdio.h>

long long a;
int b;
long c;
short d[9][1][5][20][20];

void e(long long *f, long p2) {
  *f = p2;
}

int main() {
  for (size_t g = 0; g < 9; ++g)
    for (size_t l = 0; l < 1; ++l)
      for (size_t i = 0; i < 5; ++i)
        for (size_t k = 0; k < 20; ++k)
          for (size_t j = 0; j < 20; ++j)
            d[g][l][i][k][j] = -6;

  for (_Bool g = 0; g < (_Bool)4; g = 3)
    for (unsigned l = 0; l < 7; l = 32)
      for (int j = 0; j < 20; j = 87) {
        b = (long)d[3][l][3][2][j] == 0;
        c = 4031292789 ? (unsigned)d[3][l][3][2][j] : 0;
      }
  
  e(&a, c);
  printf("%llu\n", a);

  return 0;
}

