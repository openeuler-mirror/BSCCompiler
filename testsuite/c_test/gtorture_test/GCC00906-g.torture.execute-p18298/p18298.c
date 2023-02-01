/* { dg-options "-fgnu89-inline" } */

#include <stdbool.h>
#include <stdlib.h>
extern void abort (void);
int strcmp (const char*, const char*);
char s[2048] = "a";
/* Add gnu_inline for treating this function as if it were defined in gnu89 mode when compiling in c99 mode */
__attribute__((gnu_inline))
inline bool foo(const char *str) {
  return !strcmp(s,str);
}
int main() {
int i = 0;
  while(!(foo(""))) {
    i ++;
    s[0] = '\0';
    if (i>2)
     abort ();
  }
  return 0;
}

