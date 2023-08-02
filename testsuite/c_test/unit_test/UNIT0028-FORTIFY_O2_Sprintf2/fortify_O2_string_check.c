#include <stdio.h>
#include <string.h>

#define LEN 5

int main()
{
  char dest[LEN] = "111";
  char *str = "adcde";
  int res;
  int err = 0;
  // CHECK:warning: ‘__builtin___sprintf_chk’ will always overflow; destination buffer has size 5, but size argument is 6
  sprintf(dest, "%s", str);
  res = memcmp(dest, str, LEN);
  if(res != 0) {
    err++;
  }
  return err;
}
