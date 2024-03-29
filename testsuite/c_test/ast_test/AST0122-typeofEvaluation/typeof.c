/* Test for typeof evaluation: should be at the appropriate point in
   the containing expression rather than just adding a statement.  */
#include <stdarg.h>

extern void exit (int);
extern void abort (void);

struct tree_string
{
  char str[1];
};

union tree_node
{
  struct tree_string string;
};

char *Foo (union tree_node * num_string)
{
  char *str = ((union {const char * _q; char * _nq;})
               ((const char *)(({ __typeof (num_string) const __t = num_string;  __t; })->string.str)))._nq;
  return str;
}

void *p;

void
f1 (void)
{
  int i = 0, j = -1, k = -1;
  /* typeof applied to expression with cast.  */
  j = ++i,  (void)(typeof ((int (*)[(k = ++i)])p))p;
  if (j != 1 || k != 2|| i != 2)
    abort ();
}

void
f2 (void)
{
  int i = 0, j = -1, k = -1;
  /* typeof applied to type.  */
  (j = ++i), (void)(typeof (int (*)[(k = ++i)]))p;
  if (j != 1 || k != 2 || i != 2)
    abort ();
}

void
f3 (void)
{
  int i = 0, j = -1, k = -1;
  void *q;
  /* typeof applied to expression with cast that is used.  */
  (j = ++i),(void)((typeof (1 + (int (*)[(k = ++i)])p))p);
  if (j != 1 || k != 2 || i != 2)
    abort ();
}

int a[10];
int i = 9;

void f4 (int n, ...) {
  va_list ap;
  void *p;
  va_start (ap, n);
  p = va_arg (ap, typeof (int (*)[++i]));
  if (p != a) {
    abort ();}
  if (i != n){
    abort ();}
  va_end (ap);
}

void f5 (void)
{
  int i = 0;
  int (**p)[1] = &(typeof (++i, (int (*)[i])a)){&a};
  if (*p != &a)
    abort ();
  if (i != 1)
    abort ();
}

int
main (void)
{
 int i = 1, k = 1;
  f1();
  f2();
  f3();
  f4(10, &a);
  f5();
  exit (0);
}

