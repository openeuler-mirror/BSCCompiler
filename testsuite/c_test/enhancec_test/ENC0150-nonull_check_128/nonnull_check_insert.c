#include <stdio.h>

typedef unsigned __int128 uint128;
typedef __int128 int128;

typedef union {
  int u32[4];
  uint128 u128;
  int128 i128;
} Dap128;

int main() {
  uint128 a = 1;
  Dap128 *p = &a;
  p->u128 = a;

  int128 b = 1;
  Dap128 *q = &b;
  q->i128 = b;
  printf("%llx %llx\n", (long long)(q->i128), (long long)(p->u128));
  return 0;
}
