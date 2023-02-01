/* { dg-options "-fgnu89-inline" } */

extern void exit (int);
extern void abort (void);

struct s {
  double d;
};

/* Add gnu_inline for treating this function as if it were defined in gnu89 mode when compiling in c99 mode */
__attribute__((gnu_inline)) inline struct s
sub (struct s s)
{
  s.d += 1.0;
  return s;
}

int
main ()
{
  struct s t = { 2.0 };
  t = sub (t);
  if (t.d != 3.0)
    abort ();
  exit (0);
}
