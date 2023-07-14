#include <stdio.h>

struct {
  signed f4 : 20;
  unsigned f8;
} __attribute__(()) __attribute__0, g_479[];
int g_79 = 2;
unsigned int g_815 = 7;

int func_66(int p_67, unsigned int* p_68, short p_69) {
  const unsigned int* l_429 = 0;
  for (g_79 = 0; g_79 >= -21; g_79 -= 6) {
    const unsigned int** l_560[5];
    for (int i = 0; i < 5; i++)
      l_560[i] = &l_429;
    g_479[0].f4 ^= 3 | 0;
  }
  return 0;
}
int main() {
  func_66(1, &g_815, 0);
  for (int main_i = 0; main_i < 2; main_i++)
    printf("%d\n", g_479[0].f4);
  return 0;
}
