/* { dg-options "-fgnu89-inline" } */

extern void abort (void);
extern void exit (int);

/* Add gnu_inline for treating this function as if it were defined in gnu89 mode when compiling in c99 mode */
__attribute__((gnu_inline))
inline int
f (int x)
{
  return (x + 1);
}
 
int
main (void)
{
  int a = 0 ;
 
  while ( (f(f(f(f(f(f(f(f(f(f(1))))))))))) + a < 12 )
    {
      a++;
      exit (0);
    }
  if (a != 1)
    abort();
}
