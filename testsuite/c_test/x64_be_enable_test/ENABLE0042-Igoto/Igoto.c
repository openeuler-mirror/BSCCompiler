#include <stdio.h>
void bar(int pc) {
  static const void *l[] = {&&lab0,&&lab1};
  goto *l[pc];
 lab0:
   printf("1\n");
   return;
 lab1:
   printf("2\n");
  return;
}

int main() {
  bar(0);
  bar(1);
  return 0;
}

