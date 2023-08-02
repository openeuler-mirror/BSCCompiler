#include<stdio.h>

int roundup() {
    printf("This is roundup\n");
    return 0;
}

int main() {
  int  (*proundup)()  = roundup;
  if (proundup != roundup) {
    printf("proundup=%x  roundup=%x\n", proundup, roundup);
    return 1;
  }
  if (&*proundup != roundup) {
    printf("&*proundup=%x  roundup=%x\n",&*proundup, roundup);
    return 1;
  }
  (*proundup)();
  (proundup)();
  roundup();
  return 0;
}
