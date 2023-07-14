#include <stdio.h>

struct a {
  int x;
  int *y;
};

int g = -1;

int main() {
  int c = 1;
  struct a x,y,z,o;
  o.y = &c;
  x.y = &g;
  y = x;
  z = y;
  o = z;
  g++;
  return *o.y;
}

