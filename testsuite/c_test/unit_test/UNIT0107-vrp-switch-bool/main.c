#include <stdio.h> 

int main() {
  switch((_Bool)1) {
    case (signed short int) 10489: 
      printf("10489\n");
      break;
    case (signed short int) -16267:
      printf("-16267\n");
      break;
    case (signed short int) -2268:
      printf("-2268\n");
      break;
    default:
      printf("default\n");
      break;
  }
  return 0;
}
