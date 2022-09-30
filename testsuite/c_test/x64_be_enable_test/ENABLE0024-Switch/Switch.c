#include <stdio.h>
int main ()
{
  int sel = 9;
  int res = 0;
  switch (sel)
  {
    case -1:
      res = 12;
      break;
    case 1:
      res = 1;
      break;
    case 4:
      res = 2;
    case 2:
      res = 3;
      break;
    case 3:
      res = 4;
      break;
    case 5:
      res = 5;
    case 9:
      res = 6;
      break;
    case 7:
      res = 34;
    default:
      res = 99;
  }
  printf("res = %d\n",res);
  return 0;
}
