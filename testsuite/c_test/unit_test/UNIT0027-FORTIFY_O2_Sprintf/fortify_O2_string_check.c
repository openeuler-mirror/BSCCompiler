#include <stdio.h>
#include <string.h>

#define LEN 10

int main()
{
  char dest[LEN] = "11111";
  char *str = "abcdefghi";
  int res;
  int err = 0;

  snprintf(dest, 0, "%s", str);
  printf("%s", dest);
  if(dest[0] != '1') {
    err++;
  }

  snprintf(dest, LEN, "%s", str);
  res = memcmp(dest, str, LEN - 1);
  if(res != 0) {
    err++;
  }
  return err;
}

