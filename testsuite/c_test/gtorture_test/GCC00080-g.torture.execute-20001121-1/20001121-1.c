/* { dg-options "-fgnu89-inline" } */

extern void abort (void);
extern void exit (int);

double d;

/* Add gnu_inline for treating this function as if it were defined in gnu89 mode when compiling in c99 mode */
__attribute__((gnu_inline))
__inline__ double foo (void)
{
  return d;
}

/* Add gnu_inline for treating this function as if it were defined in gnu89 mode when compiling in c99 mode */
__attribute__((gnu_inline))
__inline__ int bar (void)
{
  foo();
  return 0;
}

int main (void)
{
  if (bar ())
    abort ();
  exit (0);
}
