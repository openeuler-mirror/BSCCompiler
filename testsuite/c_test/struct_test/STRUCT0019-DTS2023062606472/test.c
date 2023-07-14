#include <stdio.h>
struct a {
  signed b;
  signed c;
} __attribute__((aligned));
#pragma pack(1)
struct {
  signed : 4;
  struct a d;
} e;
struct a f = {2, 1};
int main() {
  e.d = f;
  printf("%d\n", e.d.c);
}