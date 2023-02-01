/* { dg-options "-fgnu89-inline" } */

extern void exit (int);

/* Add gnu_inline for treating this function as if it were defined in gnu89 mode when compiling in c99 mode */
__attribute__((gnu_inline))
inline void
f (int x)
{
  int *(p[25]);
  int m[25*7];
  int i;

  for (i = 0; i < 25; i++)
    p[i] = m + x*i;

  p[1][0] = 0;
}

int
main ()
{
  f (7);
  exit (0);
}
