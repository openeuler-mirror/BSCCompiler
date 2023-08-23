#include <stdio.h>
char a[] = {255};
int main() {
  char *b = &a[0];
  a[0] && ++*b;
  if (a[0] != 0) {
    printf("%d\n", a[0]);
    return 1;
  }
  return 0;
}
