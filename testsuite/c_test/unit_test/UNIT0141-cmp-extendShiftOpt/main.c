#include <stdio.h>
long long a;
short aa[1][10][21], ab[1][10][21];
char ac[1][10][21][5][6];
int ad[1][10][21][25][6];

void func_2(long long *p1, int p2) { *p1 ^= p2; }

void func_1(short p102[][10][21], short p103[][10][21]) {
  long i_68 = 0;
  for (int i_71 = 0; i_71 < 21; i_71 += (p103[i_68][2][2] >= (unsigned long)(unsigned)(signed char)p102[i_68][2][2]) + 4) {
	ad[i_68][2][2][i_71][4] = ac[i_68][2][2][1][4];
  }
}

int main() {
  for (size_t af = 0; af < 10; ++af)
    for (size_t ag = 0; ag < 21; ++ag)
      aa[a][af][ag] = -4450;
  for (size_t af = 0; af < 10; ++af)
    for (size_t ag = 0; ag < 21; ++ag)
      ab[a][af][ag] = -29350;
  for (size_t af = 0; af < 10; ++af)
    for (size_t ag = 0; ag < 21; ++ag)
      for (size_t ah = 0; ah < 5; ++ah)
        for (size_t aj = 0; aj < 6; ++aj)
          ac[a][af][ag][ah][aj] = 7;
  func_1(aa, ab);
  for (size_t ai = 0; ai < 1; ++ai)
    for (size_t af = 0; af < 10; ++af)
      for (size_t ag = 0; ag < 21; ++ag)
        for (size_t ah = 0; ah < 5; ++ah)
          for (size_t aj = 0; aj < 6; ++aj)
		func_2(&a, ad[ai][af][ag][ah][aj]);
  printf("%llu\n", a);
}
