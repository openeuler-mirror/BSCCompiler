/* { dg-require-effective-target indirect_jumps } */

#include <setjmp.h>

extern void abort (void);

__attribute ((noinline)) void
broken_longjmp (void *p)
{
  char *buf[sizeof(jmp_buf)];
  memcpy (buf, p, sizeof(jmp_buf));
  memset (p, 0, sizeof(jmp_buf));
  /* Corrupts stack pointer...  */
  longjmp (buf, 1);
}

volatile int x = 0;
char *volatile p;
char *volatile q;

int
main ()
{
  jmp_buf buf;
  p = alloca (x);
  q = alloca (x);
  if (!setjmp(buf)) {
    broken_longjmp (buf);
  }

  /* Compute expected next alloca offset - some targets don't align properly
     and allocate too much.  */
  p = q + (q - p);

  /* Fails if stack pointer corrupted.  */
  if (p != alloca (x))
    abort ();

  return 0;
}
