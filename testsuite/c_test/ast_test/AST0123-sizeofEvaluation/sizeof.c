extern void exit (int);
extern void abort (void);

void *p;
static int i;

int foo(int n)
{
  int (*t)[n];
  i = 0;
  int j = 0;
  char b[1][n+3];           /* Variable length array.  */
  int d[3][n];              /* Variable length array.  */
  sizeof (b[(i++) + sizeof(j++)]);    /* Outer sizeof is evaluated for vla, but not the inner one.  */
  if (i != 1 || j != 0)
  {
    printf("i1 = %d, j = %d\n", i, j);
    return 1;
  }

  sizeof (d[i++]);
  if (i != 2)
  {
    printf("i2 = %d\n", i);
    return 1;
  }

  return 0;
}

int foo6(int a, int b[a][a], int (*c)[sizeof(*b)]) {
  return sizeof (*c);
}

int main (void)
{
  int i = 1, j = -1, k = -1;
  int (*a)[k = ++i];
  int (*b)[k = i++];
  int (*c)[k = i];
  if (sizeof(typeof(int(*)[2])) != sizeof(int) * 2) {
    abort ();
  }
  if (sizeof(*a) != sizeof(int) * 2 || sizeof(*b) != sizeof(int) * 2 || sizeof(*c) != sizeof(int) * 3) {
    abort();
  }

  int m[10][10];
  int (*n)[sizeof(int)*10];
  if (foo6(10, m, n) != 10*sizeof(int)*sizeof(int))
  {
    printf("sizeof(*n) = %d", sizeof (*n));
    return 1;
  }
  return foo(10);
}

