#include <stdio.h>

#pragma pack(1)
struct a {
  unsigned b : 30;
  unsigned c : 18;
  unsigned d : 25;
  unsigned e : 2;
} f[] = {5, 7, 7, 54};
static struct a g = {9, 5, 9, 2};
int main() {
  printf("%d\n", f[0].b);
  printf("%d\n", g.b);
}