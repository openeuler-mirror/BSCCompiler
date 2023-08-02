#define _GNU_SOURCE
#include <string.h> // 注意：此头文件必须包含
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
volatile void *vx;
char buf1[20];
int x;
int test(int arg, ...) {
  char buf2[20];
  va_list ap;
  memcpy (&buf2[19], "ab", 1);
  memcpy (&buf2[19], "ab", 2); // CHECK:warning: ‘__builtin___memcpy_chk’ will always overflow; destination buffer has size 1, but size argument is 2
  vx = mempcpy (&buf2[19], "ab", 1);
  vx = mempcpy (&buf2[19], "ab", 2); // CHECK:warning: ‘__builtin___memcpy_chk’ will always overflow; destination buffer has size 1, but size argument is 2 
  memmove (&buf2[18], &buf1[10], 2);
  memmove (&buf2[18], &buf1[10], 3); // CHECK:warning: ‘__builtin___memmove_chk’ will always overflow; destination buffer has size 2, but size argument is 3
  memset (&buf2[16], 'a', 4);
  memset (&buf2[15], 'b', 6); // CHECK:warning: ‘__builtin___memset_chk’ will always overflow; destination buffer has size 5, but size argument is 6
  strcpy (&buf2[18], "a");
  strcpy (&buf2[18], "aaaaaa\0\0\a"); // CHECK:warning: ‘__builtin___strcpy_chk’ will always overflow; destination buffer has size 2, but size argument is 7
  strcpy (&buf2[18], "ab"); // CHECK:warning: ‘__builtin___strcpy_chk’ will always overflow; destination buffer has size 2, but size argument is 3
  vx = stpcpy (&buf2[18], "a");
  vx = stpcpy (&buf2[18], "ab"); // CHECK:warning: ‘__builtin___stpcpy_chk’ will always overflow; destination buffer has size 2, but size argument is 3
  strncpy (&buf2[18], "a", 2);
  strncpy (&buf2[18], "a", 3); // CHECK:warning: ‘__builtin___strncpy_chk’ will always overflow; destination buffer has size 2, but size argument is 3
  strncpy (&buf2[18], "abc", 2);
  strncpy (&buf2[18], "abc", 3); // CHECK:warning: ‘__builtin___strncpy_chk’ will always overflow; destination buffer has size 2, but size argument is 3
  memset (buf2, '\0', sizeof (buf2));
  strcat (&buf2[18], "a");
  memset (buf2, '\0', sizeof (buf2));
  strcat (&buf2[18], "ab"); // CHECK:warning: ‘__builtin___strcat_chk’ will always overflow; destination buffer has size 2, but size argument is 3
  sprintf (&buf2[18], "%s", buf1);
  sprintf (&buf2[18], "%s", "a");
  sprintf (&buf2[18], "%s", "ab"); // CHECK:warning: ‘__builtin___sprintf_chk’ will always overflow; destination buffer has size 2, but size argument is 3
  sprintf (&buf2[18], "a");
  sprintf (&buf2[18], "ab"); // CHECK:warning: ‘__builtin___sprintf_chk’ will always overflow; destination buffer has size 2, but size argument is 3
  snprintf (&buf2[18], 2, "%d", x); // sprintf (&buf2[18], "aaaaa%s", "ab");
  /* N argument to snprintf is the size of the buffer.
     Although this particular call wouldn't overflow buf2,
     incorrect buffer size was passed to it and therefore
     we want a warning and runtime failure.  */
  snprintf (&buf2[18], 3, "%d", x); // CHECK:warning: ‘__builtin___snprintf_chk’ will always overflow; destination buffer has size 2, but size argument is 3
  va_start (ap, arg);
  vsprintf (&buf2[18], "a", ap);
  va_end (ap);
  va_start (ap, arg);
  vsprintf (&buf2[18], "abaaaaa", ap); // CHECK:warning: ‘__builtin___vsprintf_chk’ will always overflow; destination buffer has size 2, but size argument is 8
  va_end (ap);
  va_start (ap, arg);
  vsnprintf (&buf2[18], 2, "%sSssssssssssssssssssssss", ap);
  va_end (ap);
  va_start (ap, arg);
  /* See snprintf above.  */
  vsnprintf (&buf2[18], 3, "%s", ap); // CHECK:warning: ‘__builtin___vsnprintf_chk’ will always overflow; destination buffer has size 2, but size argument is 3
  va_end (ap);
  return 0;
}

int main() {
  return 0;
}
