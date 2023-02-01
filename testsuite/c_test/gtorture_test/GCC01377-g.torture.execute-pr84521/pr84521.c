/* { dg-require-effective-target indirect_jumps } */
/* { dg-additional-options "-fomit-frame-pointer -fno-inline" }  */

#include <setjmp.h>

extern void abort (void);

void
broken_longjmp (void *p)
{
  longjmp (p, 1);
}

volatile int x = 256;
void *volatile p = (void*)&x;
void *volatile p1;

void
test (void)
{
  jmp_buf buf;
  void *volatile q = p;

  if (!setjmp (buf))
    broken_longjmp (buf);

  /* Fails if stack pointer corrupted.  */
  if (p != q)
    abort ();
}

void
test2 (void)
{
  void *volatile q = p;
  p1 = alloca (x);
  test ();

  /* Fails if frame pointer corrupted.  */
  if (p != q)
    abort ();
}

int
main (void)
{
  void *volatile q = p;
  test ();
  test2 ();
  /* Fails if stack pointer corrupted.  */
  if (p != q)
    abort ();

  return 0;
}
