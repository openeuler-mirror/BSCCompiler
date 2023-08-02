#include <stdio.h>
struct a {
  int x;
  int y;
};

const struct b {
  int x;
  struct a y[10];
  int z;
} a = {
  .x = 1,
  .z = 2
};

int main() {
  if (!a.y[1].x) {
    printf("right\n");
  }
  return 0;
}
