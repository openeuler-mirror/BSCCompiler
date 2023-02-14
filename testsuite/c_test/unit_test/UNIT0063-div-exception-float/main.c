#include <stdlib.h>
int main() {
  if (rand()) {
   if (rand() / 0.0) {
     printf("err\n");
   }
   printf("succ\n");
  }
  return 0;
}
