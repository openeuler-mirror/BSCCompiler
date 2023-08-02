#include<stdio.h>

struct f_4 {unsigned i:4;unsigned j:4;unsigned k:4; } f_4 = {1,1,1}, F_4;

struct f_4
wack_field_4 (void)
{
  register struct f_4 u = f_4;
  printf("%d\n", u.k);
  return u;
}

int main() {
  wack_field_4 ();
}
