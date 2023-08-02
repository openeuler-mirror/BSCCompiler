#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

int g_x = 99;
int g_y = 99;

__attribute_noinline__ int abs(int n) {
  g_x = 0;
  return g_x;
}

int main(void)
{
  g_x = 1;
  g_y = abs(3);
  return g_x;
}
